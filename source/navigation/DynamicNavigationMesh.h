#pragma once

#include <memory>
#include <vector>
#include <filesystem>

#include "../navigation/NavigationMesh.h"
#include "../utils/UtilsStream.h"

class dtTileCache;
struct dtTileCacheAlloc;
struct dtTileCacheCompressor;
struct dtTileCacheMeshProcess;
struct dtTileCacheLayer;
struct dtTileCacheContourSet;
struct dtTileCachePolyMesh;

namespace WorldAssistant
{

class OffMeshConnection;
class Obstacle;

class DebugMesh;

struct TileCacheData
{
    unsigned char* data{};
    int dataSize{};
};


class DynamicNavigationMesh : public NavigationMesh, public std::enable_shared_from_this<DynamicNavigationMesh>
{
    friend class Obstacle;
    friend struct MeshProcess;
    friend class NavigationMeshBuilder;

public:
    // Constructor.
    explicit DynamicNavigationMesh(World* world);
    // Destructor.
    ~DynamicNavigationMesh() override;

    // Allocate the navigation mesh without building any tiles. Bounding box is not padded. Return true if successful.
    bool Allocate(const BoundingBox& boundingBox, unsigned maxTiles) override;
    // Build/rebuild the entire navigation mesh.
    bool Build() override;
    // Build/rebuild a portion of the navigation mesh.
    bool Build(const BoundingBox& boundingBox) override;
    // Rebuild part of the navigation mesh in the rectangular area. Return true if successful.
    bool Build(const Int32Vector2& from, const Int32Vector2& to) override;
     // Return tile data.
    std::vector<unsigned char> GetTileData(const Int32Vector2& tile) const override;
    // Add tile to navigation mesh.
    bool AddTile(const std::vector<unsigned char>& tileData) override;
    // Remove tile from navigation mesh.
    void RemoveTile(const Int32Vector2& tile) override;
    // Remove all tiles from navigation mesh.
    void RemoveAllTiles() override;

    void Dump(DebugMesh& mesh, bool triangulated = false, const BoundingBox* bounds = {});

    std::vector<unsigned char> Serialize() const;

    void Deserialize(const std::vector<unsigned char>& value);

protected:
    // Used by Obstacle class to add itself to the tile cache
    void AddObstacle(Obstacle* obstacle);
    // Used by Obstacle class to update itself.
    void ObstacleChanged(Obstacle* obstacle);
    // Used by Obstacle class to remove itself from the tile cache
    void RemoveObstacle(Obstacle* obstacle);

    // Build one tile of the navigation mesh. Return true if successful.
    int BuildTile(int x, int z, TileCacheData* tiles);
    // Build tiles in the rectangular area. Return number of built tiles.
    unsigned BuildTiles(const Int32Vector2& from, const Int32Vector2& to);

    // Off-mesh connections to be rebuilt in the mesh processor.
    std::vector<OffMeshConnection*> CollectOffMeshConnections(const BoundingBox& bounds);
    // Release the navigation mesh, query, and tile cache.
    void ReleaseNavigationMesh() override;

private:
     // Write tiles data.
    void WriteTiles(OutputStream& dest, int x, int z) const;
    // Read tiles data to the navigation mesh.
    bool ReadTiles(InputStream& source, bool silent);
     // Free the tile cache.
    void ReleaseTileCache();

    // Detour tile cache instance that works with the nav mesh.
    dtTileCache* tileCache_{};
    // Used by dtTileCache to allocate blocks of memory.
    std::unique_ptr<dtTileCacheAlloc> allocator_;
    // Used by dtTileCache to compress the original tiles to use when reconstructing for changes.
    std::unique_ptr<dtTileCacheCompressor> compressor_;
    // Mesh processor used by Detour, in this case a 'pass-through' processor.
    std::unique_ptr<dtTileCacheMeshProcess> meshProcessor_;

    // Maximum number of obstacle objects allowed.
    unsigned maxObstacles_{1024};
     // Maximum number of layers that are allowed to be constructed.
    unsigned maxLayers_{};
    // Queue of tiles to be built.
    std::vector<Int32Vector2> tileQueue_;

    bool multithreading_{ true };
};

}