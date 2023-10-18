#include <fstream>

#include "../navigation/DynamicNavigationMesh.h"
#include "../navigation/NavBuildData.h"
#include "../navigation/Obstacle.h"
#include "../scene/Scene.h"
#include "../scene/World.h"
#include "../utils/DebugMesh.h"

#include <spdlog/spdlog.h>
#include "LZ4/lz4.h"
#include "thread_pool/thread_pool.hpp"

#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourTileCache.h>
#include <DetourTileCacheBuilder.h>
#include <Recast.h>

namespace WorldAssistant
{

static const std::size_t TILECACHE_MAXLAYERS = 255u;
static const std::int32_t DEFAULT_MAX_LAYERS = 1;

struct TileCompressor : public dtTileCacheCompressor
{
    int maxCompressedSize(const int bufferSize) override
    {
        return (int)(bufferSize * 1.05f);
    }

    dtStatus compress(const unsigned char* buffer, const int bufferSize,
        unsigned char* compressed, const int /*maxCompressedSize*/, int* compressedSize) override
    {
        *compressedSize = LZ4_compress_default((const char*)buffer, (char*)compressed, bufferSize, LZ4_compressBound(bufferSize));
        return DT_SUCCESS;
    }

    dtStatus decompress(const unsigned char* compressed, const int compressedSize,
        unsigned char* buffer, const int maxBufferSize, int* bufferSize) override
    {
        *bufferSize = LZ4_decompress_safe((const char*)compressed, (char*)buffer, compressedSize, maxBufferSize);
        return *bufferSize < 0 ? DT_FAILURE : DT_SUCCESS;
    }
};

struct MeshProcess : public dtTileCacheMeshProcess
{
    DynamicNavigationMesh* owner_;
    std::vector<Vector3F> offMeshVertices_;
    std::vector<float> offMeshRadii_;
    std::vector<unsigned short> offMeshFlags_;
    std::vector<unsigned char> offMeshAreas_;
    std::vector<unsigned char> offMeshDir_;

    inline explicit MeshProcess(DynamicNavigationMesh* owner) :
        owner_(owner)
    {
    }

    void process(struct dtNavMeshCreateParams* params, unsigned char* polyAreas, unsigned short* polyFlags) override
    {
        // Update poly flags from areas.
        for (int i = 0; i < params->polyCount; ++i)
        {
            if (polyAreas[i] != RC_NULL_AREA)
                polyFlags[i] = RC_WALKABLE_AREA;
        }

        BoundingBox bounds;
        rcVcopy(&bounds.min_.x_, params->bmin);
        rcVcopy(&bounds.max_.x_, params->bmin);

        // collect off-mesh connections
        std::vector<OffMeshConnection*> offMeshConnections = owner_->CollectOffMeshConnections(bounds);

        if (offMeshConnections.size() > 0)
        {
            if (offMeshConnections.size() != offMeshRadii_.size())
            {
                ClearConnectionData();
                for (unsigned i = 0; i < offMeshConnections.size(); ++i)
                {
                    OffMeshConnection* connection = offMeshConnections[i];
        
                    offMeshVertices_.push_back(connection->GetStartPosition());
                    offMeshVertices_.push_back(connection->GetEndPosition());
                    offMeshRadii_.push_back(connection->GetRadius());
                    offMeshFlags_.push_back((unsigned short)connection->GetMask());
                    offMeshAreas_.push_back((unsigned char)connection->GetAreaID());
                    offMeshDir_.push_back((unsigned char)(connection->IsBidirectional() ? DT_OFFMESH_CON_BIDIR : 0));
                }
            }

            params->offMeshConCount = static_cast<std::int32_t>(offMeshRadii_.size());
            params->offMeshConVerts = &offMeshVertices_[0].x_;
            params->offMeshConRad = &offMeshRadii_[0];
            params->offMeshConFlags = &offMeshFlags_[0];
            params->offMeshConAreas = &offMeshAreas_[0];
            params->offMeshConDir = &offMeshDir_[0];
        }
    }

    void ClearConnectionData()
    {
        offMeshVertices_.clear();
        offMeshRadii_.clear();
        offMeshFlags_.clear();
        offMeshAreas_.clear();
        offMeshDir_.clear();
    }
};

// From the Detour/Recast Sample_TempObstacles.cpp
struct LinearAllocator : public dtTileCacheAlloc
{
    unsigned char* buffer_;
    std::size_t capacity_;
    std::size_t top_;
    std::size_t high_;

    explicit LinearAllocator(const int cap) :
        buffer_(nullptr), capacity_(0), top_(0), high_(0)
    {
        resize(cap);
    }

    ~LinearAllocator() override
    {
        dtFree(buffer_);
    }

    void resize(const std::size_t cap)
    {
        if (buffer_)
            dtFree(buffer_);
        buffer_ = (unsigned char*)dtAlloc(cap, DT_ALLOC_PERM);
        capacity_ = cap;
    }

    void reset() override
    {
        high_ = std::max(high_, top_);
        top_ = 0;
    }

    void* alloc(const size_t size) override
    {
        if (!buffer_)
            return nullptr;
        if (top_ + size > capacity_)
            return nullptr;
        unsigned char* mem = &buffer_[top_];
        top_ += size;
        return mem;
    }

    void free(void*) override
    {
    }
};

/*
    NavigationMeshBuilder
*/
class NavigationMeshBuilder
{
public:
    NavigationMeshBuilder(const std::shared_ptr<DynamicNavigationMesh>& navmesh) :
        navmesh_(navmesh)
    {
    }

    bool Build(uint32_t numTilesX, uint32_t numTilesZ)
    {
        const std::filesystem::path tempDir = "navmesh/temp/";

        std::vector<std::pair<uint32_t, uint32_t>> blocks;
        std::mutex blocksMutex;

	    // Create temp directory if not exists
	    if (!std::filesystem::exists(tempDir)) {
            spdlog::info("Temp directory was created.");
		    std::filesystem::create_directories(tempDir);
	    }

        spdlog::info("[MULTITHREADED] Start navigation mesh build! Running {} threads.", pool_.get_thread_count());

        pool_.parallelize_loop(0, numTilesX * numTilesZ,
            [this, &numTilesX, &tempDir, &blocks, &blocksMutex](const uint32_t &a, const uint32_t &b)
            {
                std::filesystem::path path = tempDir / fmt::format("temp{}_{}.bin", a, b);
                std::ofstream output(path, std::ios::out | std::ios::binary);
                if (!output.is_open()) {
                    spdlog::error("Cannot open a temp file");
                    return;
                }

                OutputFileStream stream(output);

                TileCacheData tiles[TILECACHE_MAXLAYERS];

                for (uint32_t tileIdx = a; tileIdx < b; tileIdx++) {
                    const int32_t x = tileIdx % numTilesX;
                    const int32_t z = tileIdx / numTilesX;
                    
                    const int layerCt = navmesh_->BuildTile(x, z, tiles);
                    stream.WriteInt(layerCt);

                    for (int layerIdx = 0; layerIdx < layerCt; ++layerIdx) {
                        auto& tileData = tiles[layerIdx];
                        stream.WriteInt(tileData.dataSize);
                        stream.Write(tileData.data, tileData.dataSize);

                        dtFree(tileData.data);
                        tileData.data = {};
                        tileData.dataSize = {};
                    }           
                }

                const std::lock_guard<std::mutex> lock(blocksMutex);
                blocks.push_back(std::make_pair(a, b));
            }
        );

        spdlog::info("Start loading temp files...");

        std::sort(blocks.begin(), blocks.end(), [](const std::pair<uint32_t, uint32_t>& lhs, const std::pair<uint32_t, uint32_t>& rhs) {
            return lhs.first < rhs.first;
        });

        for (const auto& [a, b] : blocks) {
            std::filesystem::path path = tempDir / fmt::format("temp{}_{}.bin", a, b);
            std::ifstream input(path, std::ios::in | std::ios::binary);
            if (!input.is_open()) {
                spdlog::error("Cannot open a temp file");
                return false;
            }

            spdlog::info("Loading blocks {} {}", a, b);

            InputFileStream stream(input);

            for (uint32_t tileIdx = a; tileIdx < b; tileIdx++) {
                const int32_t x = tileIdx % numTilesX;
                const int32_t z = tileIdx / numTilesX; 

                navmesh_->tileCache_->removeTile(navmesh_->navMesh_->getTileRefAt(x, z, 0), nullptr, nullptr);

                const int layerCt = stream.ReadInt();
                for (int layerIdx = 0; layerIdx < layerCt; ++layerIdx) {
                    const int dataSize = stream.ReadInt();

                    void* data = dtAlloc(dataSize, DT_ALLOC_PERM);
                    stream.Read(data, dataSize);

                    dtCompressedTileRef tileRef;
                    int status = navmesh_->tileCache_->addTile(static_cast<unsigned char*>(data), dataSize, DT_COMPRESSEDTILE_FREE_DATA, &tileRef);
                    if (dtStatusFailed((dtStatus)status))
                    {
                        dtFree(data);
                        data = nullptr;
                    }
                }

                navmesh_->tileCache_->buildNavMeshTilesAt(x, z, navmesh_->navMesh_);
            }            
        }

        // For a full build it's necessary to update the nav mesh
        // not doing so will cause dependent components to crash, like CrowdManager
        navmesh_->tileCache_->update(0, navmesh_->navMesh_);

        return true;
    }   

private:
    thread_pool pool_;

    std::shared_ptr<DynamicNavigationMesh> navmesh_;
};

DynamicNavigationMesh::DynamicNavigationMesh(World* world) :
    NavigationMesh(world),
    maxLayers_(DEFAULT_MAX_LAYERS)
{
    // 64 is the largest tile-size that DetourTileCache will tolerate without silently failing
    tileSize_ = 64;
    partitionType_ = NAVMESH_PARTITION_WATERSHED;
    allocator_ = std::make_unique<LinearAllocator>(32000); //32kb to start
    compressor_ = std::make_unique<TileCompressor>();
    meshProcessor_ = std::make_unique<MeshProcess>(this);
}

DynamicNavigationMesh::~DynamicNavigationMesh()
{
    ReleaseNavigationMesh();
}

bool DynamicNavigationMesh::Allocate(const BoundingBox& boundingBox, unsigned maxTiles)
{
    Scene* scene = world_->GetScene();
    assert(scene);

    // Release existing navigation data and zero the bounding box
    ReleaseNavigationMesh();

    boundingBox_ = boundingBox;
    maxTiles = NextPowerOfTwo(maxTiles);

    // Calculate number of tiles
    int gridW = 0, gridH = 0;
    float tileEdgeLength = (float)tileSize_ * cellSize_;
    rcCalcGridSize(&boundingBox_.min_.x_, &boundingBox_.max_.x_, cellSize_, &gridW, &gridH);
    numTilesX_ = (gridW + tileSize_ - 1) / tileSize_;
    numTilesZ_ = (gridH + tileSize_ - 1) / tileSize_;

    // Calculate max number of polygons, 22 bits available to identify both tile & polygon within tile
    unsigned tileBits = LogBaseTwo(maxTiles);
    unsigned maxPolys = 1u << (22 - tileBits);

    dtNavMeshParams params;     // NOLINT(hicpp-member-init)
    rcVcopy(params.orig, &boundingBox_.min_.x_);
    params.tileWidth = tileEdgeLength;
    params.tileHeight = tileEdgeLength;
    params.maxTiles = maxTiles;
    params.maxPolys = maxPolys;

    navMesh_ = dtAllocNavMesh();
    if (!navMesh_)
    {
        spdlog::error("Could not allocate navigation mesh");
        return false;
    }

    if (dtStatusFailed(navMesh_->init(&params)))
    {
        spdlog::error("Could not initialize navigation mesh");
        ReleaseNavigationMesh();
        return false;
    }

    dtTileCacheParams tileCacheParams;      // NOLINT(hicpp-member-init)
    memset(&tileCacheParams, 0, sizeof(tileCacheParams));
    rcVcopy(tileCacheParams.orig, &boundingBox_.min_.x_);
    tileCacheParams.ch = cellHeight_;
    tileCacheParams.cs = cellSize_;
    tileCacheParams.width = tileSize_;
    tileCacheParams.height = tileSize_;
    tileCacheParams.maxSimplificationError = edgeMaxError_;
    tileCacheParams.maxTiles = maxTiles * maxLayers_;
    tileCacheParams.maxObstacles = maxObstacles_;
    // Settings from NavigationMesh
    tileCacheParams.walkableClimb = agentMaxClimb_;
    tileCacheParams.walkableHeight = agentHeight_;
    tileCacheParams.walkableRadius = agentRadius_;

    tileCache_ = dtAllocTileCache();
    if (!tileCache_)
    {
        spdlog::error("Could not allocate tile cache");
        ReleaseNavigationMesh();
        return false;
    }

    if (dtStatusFailed(tileCache_->init(&tileCacheParams, allocator_.get(), compressor_.get(), meshProcessor_.get())))
    {
        spdlog::error("Could not initialize tile cache");
        ReleaseNavigationMesh();
        return false;
    }

    spdlog::debug("Allocated empty navigation mesh with max {} tiles", maxTiles);

    // Scan for obstacles to insert into us
    for (const auto& obstacle : scene->GetObstacles()) {
        if (obstacle->IsEnabled()) {
            AddObstacle(obstacle.get());
        }
    }    
   
    return true;
}

bool DynamicNavigationMesh::Build()
{
    Scene* scene = world_->GetScene();
    assert(scene);   

    // Release existing navigation data and zero the bounding box
    ReleaseNavigationMesh();

    boundingBox_.Merge(scene->GetBounds());

    // Expand bounding box by padding
    boundingBox_.min_ -= padding_;
    boundingBox_.max_ += padding_;

    {
        // Calculate number of tiles
        int gridW = 0, gridH = 0;
        float tileEdgeLength = (float)tileSize_ * cellSize_;
        rcCalcGridSize(&boundingBox_.min_.x_, &boundingBox_.max_.x_, cellSize_, &gridW, &gridH);
        numTilesX_ = (gridW + tileSize_ - 1) / tileSize_;
        numTilesZ_ = (gridH + tileSize_ - 1) / tileSize_;

        // Calculate max. number of tiles and polygons, 22 bits available to identify both tile & polygon within tile
        unsigned maxTiles = NextPowerOfTwo((unsigned)(numTilesX_ * numTilesZ_)) * maxLayers_;
        unsigned tileBits = LogBaseTwo(maxTiles);
        unsigned maxPolys = 1u << (22 - tileBits);

        spdlog::info("Max Tiles {}; Max Polys {}", maxTiles, maxPolys);
        spdlog::info("Tiles {} x {}", numTilesX_, numTilesZ_);

        dtNavMeshParams params;     // NOLINT(hicpp-member-init)
        rcVcopy(params.orig, &boundingBox_.min_.x_);
        params.tileWidth = tileEdgeLength;
        params.tileHeight = tileEdgeLength;
        params.maxTiles = maxTiles;
        params.maxPolys = maxPolys;

        navMesh_ = dtAllocNavMesh();
        if (!navMesh_)
        {
            spdlog::error("Could not allocate navigation mesh");
            return false;
        }

        if (dtStatusFailed(navMesh_->init(&params)))
        {
            spdlog::error("Could not initialize navigation mesh");
            ReleaseNavigationMesh();
            return false;
        }

        dtTileCacheParams tileCacheParams;      // NOLINT(hicpp-member-init)
        memset(&tileCacheParams, 0, sizeof(tileCacheParams));
        rcVcopy(tileCacheParams.orig, &boundingBox_.min_.x_);
        tileCacheParams.ch = cellHeight_;
        tileCacheParams.cs = cellSize_;
        tileCacheParams.width = tileSize_;
        tileCacheParams.height = tileSize_;
        tileCacheParams.maxSimplificationError = edgeMaxError_;
        tileCacheParams.maxTiles = maxTiles;
        tileCacheParams.maxObstacles = maxObstacles_;
        // Settings from NavigationMesh
        tileCacheParams.walkableClimb = agentMaxClimb_;
        tileCacheParams.walkableHeight = agentHeight_;
        tileCacheParams.walkableRadius = agentRadius_;

        tileCache_ = dtAllocTileCache();
        if (!tileCache_)
        {
            spdlog::error("Could not allocate tile cache");
            ReleaseNavigationMesh();
            return false;
        }

        if (dtStatusFailed(tileCache_->init(&tileCacheParams, allocator_.get(), compressor_.get(), meshProcessor_.get())))
        {
            spdlog::error("Could not initialize tile cache");
            ReleaseNavigationMesh();
            return false;
        }
    
 
        NavigationMeshBuilder builder(shared_from_this());
        builder.Build(numTilesX_, numTilesZ_); 

        spdlog::debug("Built navigation mesh");

        // Scan for obstacles to insert into us
        for (const auto& obstacle : scene->GetObstacles()) {
            if (obstacle->IsEnabled()) {
                AddObstacle(obstacle.get());
            }
        }  

        return true;
    }
}

bool DynamicNavigationMesh::Build(const BoundingBox& boundingBox)
{
    if (!navMesh_)
    {
        spdlog::error("Navigation mesh must first be built fully before it can be partially rebuilt");
        return false;
    }

    float tileEdgeLength = (float)tileSize_ * cellSize_;

    int sx = Clamp((int)((boundingBox.min_.x_ - boundingBox_.min_.x_) / tileEdgeLength), 0, numTilesX_ - 1);
    int sz = Clamp((int)((boundingBox.min_.z_ - boundingBox_.min_.z_) / tileEdgeLength), 0, numTilesZ_ - 1);
    int ex = Clamp((int)((boundingBox.max_.x_ - boundingBox_.min_.x_) / tileEdgeLength), 0, numTilesX_ - 1);
    int ez = Clamp((int)((boundingBox.max_.z_ - boundingBox_.min_.z_) / tileEdgeLength), 0, numTilesZ_ - 1);

    unsigned numTiles = BuildTiles(Int32Vector2(sx, sz), Int32Vector2(ex, ez));

    spdlog::debug("Rebuilt {} tiles of the navigation mesh", numTiles);
    return true;
}

bool DynamicNavigationMesh::Build(const Int32Vector2& from, const Int32Vector2& to)
{
    if (!navMesh_)
    {
        spdlog::error("Navigation mesh must first be built fully before it can be partially rebuilt");
        return false;
    }

    unsigned numTiles = BuildTiles(from, to);

    spdlog::debug("Rebuilt {} tiles of the navigation mesh", numTiles);
    return true;
}

std::vector<unsigned char> DynamicNavigationMesh::GetTileData(const Int32Vector2& tile) const
{
    std::vector<unsigned char> ret;
    OutputMemoryStream stream(ret);

    dtCompressedTileRef tiles[TILECACHE_MAXLAYERS];
    WriteTiles(stream, tile.x_, tile.y_, tiles);

    return ret;
}

bool DynamicNavigationMesh::AddTile(const std::vector<unsigned char>& tileData)
{
    InputMemoryStream stream(tileData);
    return ReadTiles(stream, false);
}

void DynamicNavigationMesh::RemoveTile(const Int32Vector2& tile)
{
    if (!navMesh_)
        return;

    dtCompressedTileRef existing[TILECACHE_MAXLAYERS];
    const int existingCt = tileCache_->getTilesAt(tile.x_, tile.y_, existing, maxLayers_);
    for (int i = 0; i < existingCt; ++i)
    {
        unsigned char* data = nullptr;
        if (!dtStatusFailed(tileCache_->removeTile(existing[i], &data, nullptr)) && data != nullptr)
            dtFree(data);
    }

    NavigationMesh::RemoveTile(tile);
}

void DynamicNavigationMesh::RemoveAllTiles()
{
    int numTiles = tileCache_->getTileCount();
    for (int i = 0; i < numTiles; ++i)
    {
        const dtCompressedTile* tile = tileCache_->getTile(i);
        assert(tile);
        if (tile->header)
            tileCache_->removeTile(tileCache_->getTileRef(tile), nullptr, nullptr);
    }

    NavigationMesh::RemoveAllTiles();
}

std::size_t DynamicNavigationMesh::GetEffectiveTilesCount() const
{
    if (tileCache_) {
        return static_cast<std::size_t>(tileCache_->getTileCount());
    }
    
    return 0u;
}

bool DynamicNavigationMesh::Dump(DebugMesh& mesh, bool triangulated, const BoundingBox* bounds)
{
    if (!navMesh_) {
        return false;
    }

    const dtNavMesh* navMesh = navMesh_;

    const auto InsertTile = [&mesh, triangulated](const dtMeshTile* tile) {
        for (int i = 0; i < tile->header->polyCount; ++i) {
            dtPoly* poly = tile->polys + i;
            if (poly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION) { // Skip off-mesh links.
			    continue;
            }

            const dtPolyDetail* pd = &tile->detailMeshes[i];
            for (int j = 0; j < pd->triCount; ++j)
		    {
			    const unsigned char* t = &tile->detailTris[(pd->triBase+j)*4];
			    for (int k = 0; k < 3; ++k)
			    {
				    if (t[k] < poly->vertCount) {     
                        mesh.AddVertex(Vector3F(&tile->verts[poly->verts[t[k]]*3]));
                    }
                    else {
                        mesh.AddVertex(&tile->detailVerts[(pd->vertBase+t[k]-poly->vertCount)*3]);
                    }					   
			    }
		    }

            for (unsigned j = 0; j < poly->vertCount; ++j) {
                if (triangulated) {
                    
                }
                else {
                    mesh.AddLine(*reinterpret_cast<const Vector3F*>(&tile->verts[poly->verts[j] * 3]),
                        *reinterpret_cast<const Vector3F*>(&tile->verts[poly->verts[(j + 1) % poly->vertCount] * 3]));
                }
            }
        }
    };

    if (bounds) {
        int32_t minTileX, minTileY;
        navMesh->calcTileLoc(&bounds->min_.x_, &minTileX, &minTileY);
        int32_t maxTileX, maxTileY;
        navMesh->calcTileLoc(&bounds->max_.x_, &maxTileX, &maxTileY);

        if (minTileX > maxTileX || minTileY > maxTileY) {
            return false;
        }

        const int32_t tilesH = maxTileX - minTileX;
        const int32_t tilesV = maxTileY - minTileY;

        dtMeshTile const* tiles[TILECACHE_MAXLAYERS];

        for (int32_t tileY = 0; tileY <= tilesV; ++tileY) {
            for (int32_t tileX = 0; tileX <= tilesH; ++tileX) {
                const int32_t tilesNum = navMesh->getTilesAt(minTileX + tileX, minTileY + tileY, tiles, TILECACHE_MAXLAYERS);
                for (int32_t i = 0; i < tilesNum; ++i) {
                    const dtMeshTile* tile = tiles[i];
                    if (tile && tile->header) {
                        InsertTile(tile);
                    }
                }                
            }
        }
    }
    else {
        for (int i = 0; i < navMesh->getMaxTiles(); ++i) {
            const dtMeshTile* tile = navMesh->getTile(i);
            if (tile && tile->header) {
                InsertTile(tile);
            }
        }  
    }
    
    return true;
}

bool DynamicNavigationMesh::Serialize(OutputStream& stream) const
{
    if (navMesh_ && tileCache_)
    {
        stream.WriteBoundingBox(boundingBox_);
        stream.WriteInt(numTilesX_);
        stream.WriteInt(numTilesZ_);

        const dtNavMeshParams* params = navMesh_->getParams();
        stream.Write(params, sizeof(dtNavMeshParams));

        const dtTileCacheParams* tcParams = tileCache_->getParams();
        stream.Write(tcParams, sizeof(dtTileCacheParams));

        dtCompressedTileRef tiles[TILECACHE_MAXLAYERS];

        for (int z = 0; z < numTilesZ_; ++z) {
            for (int x = 0; x < numTilesX_; ++x) {
                if (!WriteTiles(stream, x, z, tiles)) {
                    spdlog::error("An internal navmesh serialization error. Aborting the serialization process.");
                    return false;
                }
            }
        }
    }

    return true;
}

bool DynamicNavigationMesh::Deserialize(InputStream& stream)
{
    ReleaseNavigationMesh();

    boundingBox_ = stream.ReadBoundingBox();
    numTilesX_ = stream.ReadInt();
    numTilesZ_ = stream.ReadInt();

    dtNavMeshParams params;     // NOLINT(hicpp-member-init)
    stream.Read(&params, sizeof(dtNavMeshParams));

    navMesh_ = dtAllocNavMesh();
    if (!navMesh_)
    {
        spdlog::error("Could not allocate navigation mesh");
        return false;
    }

    if (dtStatusFailed(navMesh_->init(&params)))
    {
        spdlog::error("Could not initialize navigation mesh");
        ReleaseNavigationMesh();
        return false;
    }

    dtTileCacheParams tcParams;     // NOLINT(hicpp-member-init)
    stream.Read(&tcParams, sizeof(tcParams));

    tileCache_ = dtAllocTileCache();
    if (!tileCache_)
    {
        spdlog::error("Could not allocate tile cache");
        ReleaseNavigationMesh();
        return false;
    }
    if (dtStatusFailed(tileCache_->init(&tcParams, allocator_.get(), compressor_.get(), meshProcessor_.get())))
    {
        spdlog::error("Could not initialize tile cache");
        ReleaseNavigationMesh();
        return false;
    }

    return ReadTiles(stream, true); 
}

void DynamicNavigationMesh::AddObstacle(Obstacle* obstacle)
{
    if (tileCache_)
    {
        float pos[3];
        auto obsPos = obstacle->GetWorldPosition();
        rcVcopy(pos, &obsPos.x_);
        dtObstacleRef refHolder;

        // Because dtTileCache doesn't process obstacle requests while updating tiles
        // it's necessary update until sufficient request space is available
        while (tileCache_->isObstacleQueueFull())
            tileCache_->update(1, navMesh_);

        if (dtStatusFailed(tileCache_->addObstacle(pos, obstacle->GetRadius(), obstacle->GetHeight(), &refHolder)))
        {
            spdlog::error("Failed to add obstacle");
            return;
        }
        obstacle->obstacleId_ = refHolder;
        assert(refHolder > 0);       
    }
}

void DynamicNavigationMesh::ObstacleChanged(Obstacle* obstacle)
{
    if (tileCache_)
    {
        RemoveObstacle(obstacle);
        AddObstacle(obstacle);
    }
}

void DynamicNavigationMesh::RemoveObstacle(Obstacle* obstacle)
{
    if (tileCache_ && obstacle->obstacleId_ > 0)
    {
        // Because dtTileCache doesn't process obstacle requests while updating tiles
        // it's necessary update until sufficient request space is available
        while (tileCache_->isObstacleQueueFull())
            tileCache_->update(1, navMesh_);

        if (dtStatusFailed(tileCache_->removeObstacle(obstacle->obstacleId_)))
        {
            spdlog::error("Failed to remove obstacle");
            return;
        }
        obstacle->obstacleId_ = 0;        
    }
}

int DynamicNavigationMesh::BuildTile(int x, int z, TileCacheData* tiles)
{
    const auto tileBoundingBox = GetTileBoundingBox(Int32Vector2(x, z));

    DynamicNavBuildData build(allocator_.get());

    rcConfig cfg;   // NOLINT(hicpp-member-init)
    memset(&cfg, 0, sizeof cfg);
    cfg.cs = cellSize_;
    cfg.ch = cellHeight_;
    cfg.walkableSlopeAngle = agentMaxSlope_;
    cfg.walkableHeight = (int)ceilf(agentHeight_ / cfg.ch);
    cfg.walkableClimb = (int)floorf(agentMaxClimb_ / cfg.ch);
    cfg.walkableRadius = (int)ceilf(agentRadius_ / cfg.cs);
    cfg.maxEdgeLen = (int)(edgeMaxLength_ / cellSize_);
    cfg.maxSimplificationError = edgeMaxError_;
    cfg.minRegionArea = (int)sqrtf(regionMinSize_);
    cfg.mergeRegionArea = (int)sqrtf(regionMergeSize_);
    cfg.maxVertsPerPoly = 6;
    cfg.tileSize = tileSize_;
    cfg.borderSize = cfg.walkableRadius + 3; // Add padding
    cfg.width = cfg.tileSize + cfg.borderSize * 2;
    cfg.height = cfg.tileSize + cfg.borderSize * 2;
    cfg.detailSampleDist = detailSampleDistance_ < 0.9f ? 0.0f : cellSize_ * detailSampleDistance_;
    cfg.detailSampleMaxError = cellHeight_ * detailSampleMaxError_;

    rcVcopy(cfg.bmin, &tileBoundingBox.min_.x_);
    rcVcopy(cfg.bmax, &tileBoundingBox.max_.x_);
    cfg.bmin[0] -= cfg.borderSize * cfg.cs;
    cfg.bmin[2] -= cfg.borderSize * cfg.cs;
    cfg.bmax[0] += cfg.borderSize * cfg.cs;
    cfg.bmax[2] += cfg.borderSize * cfg.cs;

    BoundingBox expandedBox(*reinterpret_cast<Vector3F*>(cfg.bmin), *reinterpret_cast<Vector3F*>(cfg.bmax));
    GetTileGeometry(&build, expandedBox);

    if (build.vertices_.empty() || build.indices_.empty())
        return 0; // Nothing to do

    build.heightField_ = rcAllocHeightfield();
    if (!build.heightField_)
    {
        spdlog::error("Could not allocate heightfield");
        return 0;
    }

    if (!rcCreateHeightfield(build.ctx_, *build.heightField_, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs,
        cfg.ch))
    {
        spdlog::error("Could not create heightfield");
        return 0;
    }

    const std::int32_t numTriangles = static_cast<std::int32_t>(build.indices_.size()) / 3;
    std::unique_ptr<unsigned char[]> triAreas(new unsigned char[numTriangles]);
    memset(triAreas.get(), 0, numTriangles);

    rcMarkWalkableTriangles(build.ctx_, cfg.walkableSlopeAngle, &build.vertices_[0].x_, static_cast<std::int32_t>(build.vertices_.size()),
        &build.indices_[0], numTriangles, triAreas.get());
    rcRasterizeTriangles(build.ctx_, &build.vertices_[0].x_, static_cast<std::int32_t>(build.vertices_.size()), &build.indices_[0],
        triAreas.get(), numTriangles, *build.heightField_, cfg.walkableClimb);
    rcFilterLowHangingWalkableObstacles(build.ctx_, cfg.walkableClimb, *build.heightField_);

    rcFilterLedgeSpans(build.ctx_, cfg.walkableHeight, cfg.walkableClimb, *build.heightField_);
    rcFilterWalkableLowHeightSpans(build.ctx_, cfg.walkableHeight, *build.heightField_);

    build.compactHeightField_ = rcAllocCompactHeightfield();
    if (!build.compactHeightField_)
    {
        spdlog::error("Could not allocate create compact heightfield");
        return 0;
    }
    if (!rcBuildCompactHeightfield(build.ctx_, cfg.walkableHeight, cfg.walkableClimb, *build.heightField_,
        *build.compactHeightField_))
    {
        spdlog::error("Could not build compact heightfield");
        return 0;
    }
    if (!rcErodeWalkableArea(build.ctx_, cfg.walkableRadius, *build.compactHeightField_))
    {
        spdlog::error("Could not erode compact heightfield");
        return 0;
    }

    // area volumes
    for (unsigned i = 0; i < build.navAreas_.size(); ++i)
        rcMarkBoxArea(build.ctx_, &build.navAreas_[i].bounds_.min_.x_, &build.navAreas_[i].bounds_.max_.x_,
            build.navAreas_[i].areaID_, *build.compactHeightField_);

    if (this->partitionType_ == NAVMESH_PARTITION_WATERSHED)
    {
        if (!rcBuildDistanceField(build.ctx_, *build.compactHeightField_))
        {
            spdlog::error("Could not build distance field");
            return 0;
        }
        if (!rcBuildRegions(build.ctx_, *build.compactHeightField_, cfg.borderSize, cfg.minRegionArea,
            cfg.mergeRegionArea))
        {
            spdlog::error("Could not build regions");
            return 0;
        }
    }
    else
    {
        if (!rcBuildRegionsMonotone(build.ctx_, *build.compactHeightField_, cfg.borderSize, cfg.minRegionArea, cfg.mergeRegionArea))
        {
            spdlog::error("Could not build monotone regions");
            return 0;
        }
    }

    build.heightFieldLayers_ = rcAllocHeightfieldLayerSet();
    if (!build.heightFieldLayers_)
    {
        spdlog::error("Could not allocate height field layer set");
        return 0;
    }

    if (!rcBuildHeightfieldLayers(build.ctx_, *build.compactHeightField_, cfg.borderSize, cfg.walkableHeight,
        *build.heightFieldLayers_))
    {
        spdlog::error("Could not build height field layers. See {}:{}", x, z);
        return 0;
    }

    int retCt = 0;
    for (int i = 0; i < build.heightFieldLayers_->nlayers; ++i)
    {
        dtTileCacheLayerHeader header;      // NOLINT(hicpp-member-init)
        header.magic = DT_TILECACHE_MAGIC;
        header.version = DT_TILECACHE_VERSION;
        header.tx = x;
        header.ty = z;
        header.tlayer = i;

        rcHeightfieldLayer* layer = &build.heightFieldLayers_->layers[i];

        // Tile info.
        rcVcopy(header.bmin, layer->bmin);
        rcVcopy(header.bmax, layer->bmax);
        header.width = (unsigned char)layer->width;
        header.height = (unsigned char)layer->height;
        header.minx = (unsigned char)layer->minx;
        header.maxx = (unsigned char)layer->maxx;
        header.miny = (unsigned char)layer->miny;
        header.maxy = (unsigned char)layer->maxy;
        header.hmin = (unsigned short)layer->hmin;
        header.hmax = (unsigned short)layer->hmax;

        if (dtStatusFailed(
            dtBuildTileCacheLayer(compressor_.get()/*compressor*/, &header, layer->heights, layer->areas/*areas*/, layer->cons,
                &(tiles[retCt].data), &tiles[retCt].dataSize)))
        {
            spdlog::error("Failed to build tile cache layers");
            return 0;
        }
        else
            ++retCt;
    }    

    return retCt;
}

unsigned DynamicNavigationMesh::BuildTiles(const Int32Vector2& from, const Int32Vector2& to)
{
    unsigned numTiles = 0;

    for (int z = from.y_; z <= to.y_; ++z)
    {
        for (int x = from.x_; x <= to.x_; ++x)
        {
            dtCompressedTileRef existing[TILECACHE_MAXLAYERS];
            const int existingCt = tileCache_->getTilesAt(x, z, existing, maxLayers_);
            for (int i = 0; i < existingCt; ++i)
            {
                unsigned char* data = nullptr;
                if (!dtStatusFailed(tileCache_->removeTile(existing[i], &data, nullptr)) && data != nullptr)
                    dtFree(data);
            }

            tileCache_->removeTile(navMesh_->getTileRefAt(x, z, 0), nullptr, nullptr);

            TileCacheData tiles[TILECACHE_MAXLAYERS];
            int layerCt = BuildTile(x, z, tiles);
            for (int i = 0; i < layerCt; ++i)
            {
                dtCompressedTileRef tileRef;
                int status = tileCache_->addTile(tiles[i].data, tiles[i].dataSize, DT_COMPRESSEDTILE_FREE_DATA, &tileRef);
                if (dtStatusFailed((dtStatus)status))
                {
                    dtFree(tiles[i].data);
                    tiles[i].data = nullptr;
                }
                else
                {
                    tileCache_->buildNavMeshTile(tileRef, navMesh_);
                    ++numTiles;
                }
            }
        }
    }

    return numTiles;
}

std::vector<OffMeshConnection*> DynamicNavigationMesh::CollectOffMeshConnections(const BoundingBox& bounds)
{
    std::vector<OffMeshConnection*> connections;

    Scene* scene = world_->GetScene();
    assert(scene);

    for (const auto& connection : scene->GetOffMeshConnections()) {
        if (connection->IsEnabled()) {
            connections.push_back(connection.get());
        }
    }   

    return connections;
}

void DynamicNavigationMesh::ReleaseNavigationMesh()
{
    NavigationMesh::ReleaseNavigationMesh();
    ReleaseTileCache();
}

bool DynamicNavigationMesh::WriteTiles(OutputStream& dest, int x, int z, dtCompressedTileRef* tiles) const
{    
    const int ct = tileCache_->getTilesAt(x, z, tiles, TILECACHE_MAXLAYERS);
    for (int i = 0; i < ct; ++i) {
        const dtCompressedTile* tile = tileCache_->getTileByRef(tiles[i]);
        if (!tile || !tile->header || !tile->dataSize) {
            continue; // Don't write "void-space" tiles
        }

        dest.Write(tile->header, sizeof(dtTileCacheLayerHeader));
        dest.WriteInt(tile->dataSize);

        if (!dest.Write(tile->data, (unsigned)tile->dataSize)) {
            return false;
        }
    }
    
    return true;
}

bool DynamicNavigationMesh::ReadTiles(InputStream& source, bool silent)
{
    tileQueue_.clear();

    while (!source.Eof())
    {
        dtTileCacheLayerHeader header;
        source.Read(&header, sizeof(dtTileCacheLayerHeader));
        const int dataSize = source.ReadInt();

        auto* data = (unsigned char*)dtAlloc(dataSize, DT_ALLOC_PERM);
        if (!data)
        {
            spdlog::error("Could not allocate data for navigation mesh tile");
            return false;
        }

        source.Read(data, (unsigned)dataSize);

        if (dtStatusFailed(tileCache_->addTile(data, dataSize, DT_TILE_FREE_DATA, nullptr)))
        {
            spdlog::error("Failed to add tile");
            dtFree(data);
            return false;
        }

        const auto tileIdx = Int32Vector2(header.tx, header.ty);
        if (tileQueue_.empty() || tileQueue_.back() != tileIdx)
            tileQueue_.push_back(tileIdx);
    }

    for (unsigned i = 0; i < tileQueue_.size(); ++i)
        tileCache_->buildNavMeshTilesAt(tileQueue_[i].x_, tileQueue_[i].y_, navMesh_);

    tileCache_->update(0, navMesh_);
   
    return true;
}

void DynamicNavigationMesh::ReleaseTileCache()
{
    dtFreeTileCache(tileCache_);
    tileCache_ = nullptr;
}

}