#pragma once

#include <vector>

#include "../utils/MathUtils.h"

#ifdef DT_POLYREF64
using dtPolyRef = uint64_t;
#else
using dtPolyRef = unsigned int;
#endif

class dtNavMesh;
class dtNavMeshQuery;
class dtQueryFilter;

namespace WorldAssistant
{

class World;
class Scene;
struct NavBuildData;
struct FindPathData;

enum NavmeshPartitionType
{
    NAVMESH_PARTITION_WATERSHED = 0,
    NAVMESH_PARTITION_MONOTONE
};

// A flag representing the type of path point- none, the start of a path segment, the end of one, or an off-mesh connection.
enum NavigationPathPointFlag
{
    NAVPATHFLAG_NONE = 0,
    NAVPATHFLAG_START = 0x01,
    NAVPATHFLAG_END = 0x02,
    NAVPATHFLAG_OFF_MESH = 0x04
};

struct NavigationPathPoint
{
    // World-space position of the path point.
    Vector3F position_;
    // Detour flag.
    NavigationPathPointFlag flag_;
    // Detour area ID.
    unsigned char areaID_;
};

class NavigationMesh
{
public:
    explicit NavigationMesh(World* world) noexcept;

    virtual ~NavigationMesh();

    // Allocate the navigation mesh without building any tiles. Bounding box is not padded. Return true if successful.
    virtual bool Allocate(const BoundingBox& boundingBox, unsigned maxTiles);
    // Rebuild the navigation mesh. Return true if successful.
    virtual bool Build();
    // Rebuild part of the navigation mesh contained by the world-space bounding box. Return true if successful.
    virtual bool Build(const BoundingBox& boundingBox);
    // Rebuild part of the navigation mesh in the rectangular area. Return true if successful.
    virtual bool Build(const Int32Vector2& from, const Int32Vector2& to);
    // Return tile data.
    virtual std::vector<unsigned char> GetTileData(const Int32Vector2& tile) const;
    // Add tile to navigation mesh.
    virtual bool AddTile(const std::vector<unsigned char>& tileData);
    // Remove tile from navigation mesh.
    virtual void RemoveTile(const Int32Vector2& tile);
    // Remove all tiles from navigation mesh.
    virtual void RemoveAllTiles();
    // Return whether the navigation mesh has tile.
    bool HasTile(const Int32Vector2& tile) const;

    // Find the nearest point on the navigation mesh to a given point. Extents specifies how far out from the specified point to check along each axis.
    Vector3F FindNearestPoint(const Vector3F& point,const Vector3F& extents, const dtQueryFilter* filter = nullptr, dtPolyRef* nearestRef = nullptr);
    // Find a path between world space points. Return non-empty list of points if successful. Extents specifies how far off the navigation mesh the points can be.
    void FindPath(std::vector<Vector3F>& dest, const Vector3F& start, const Vector3F& end, const Vector3F& extents, const dtQueryFilter* filter = nullptr);
    // Find a path between world space points. Return non-empty list of navigation path points if successful. Extents specifies how far off the navigation mesh the points can be.
    void FindPath(std::vector<NavigationPathPoint>& dest, const Vector3F& start, const Vector3F& end, const Vector3F& extents, const dtQueryFilter* filter = nullptr);

    // Return bounding box of the tile in the node space.
    BoundingBox GetTileBoundingBox(const Int32Vector2& tile) const;

    // Return number of tiles.
    Int32Vector2 GetNumTiles() const { return Int32Vector2(numTilesX_, numTilesZ_); }

protected:
    // Build one tile of the navigation mesh. Return true if successful.
    virtual bool BuildTile(int x, int z);

     // Get geometry data within a bounding box.
    void GetTileGeometry(NavBuildData* build, BoundingBox& box);

     // Ensure that the navigation mesh query is initialized. Return true if successful.
    bool InitializeQuery();
     // Release the navigation mesh and the query.
    virtual void ReleaseNavigationMesh();

    World* world_; 

	// Detour navigation mesh.
    dtNavMesh* navMesh_{};
    // Detour navigation mesh query.
    dtNavMeshQuery* navMeshQuery_{};
     // Detour navigation mesh query filter.
    dtQueryFilter* queryFilter_{};
    // Temporary data for finding a path.
    FindPathData* pathData_{};

    // Tile size.
    int tileSize_{128};
    // Cell size.
    float cellSize_{0.3f};
    // Cell height.
    float cellHeight_{0.2f};
    // Navigation agent height.
    float agentHeight_{2.0f};
    // Navigation agent radius.
    float agentRadius_{0.6f};
     // Navigation agent max vertical climb.
    float agentMaxClimb_{0.4f};
    // Navigation agent max slope.
    float agentMaxSlope_{45.0f};

    // Region minimum size.
    float regionMinSize_{8.0f};
    // Region merge size.
    float regionMergeSize_{20.0f};
    // Edge max length.
    float edgeMaxLength_{12.0f};
    // Edge max error.
    float edgeMaxError_{1.3f};
    // Detail sampling distance.
    float detailSampleDistance_{6.0f};
    // Detail sampling maximum error.
    float detailSampleMaxError_{1.0f};
    // Bounding box padding.
    Vector3F padding_;
     // Number of tiles in X direction.
    int numTilesX_{};
    // Number of tiles in Z direction.
    int numTilesZ_{};

    // Whole navigation mesh bounding box.
    BoundingBox boundingBox_;
    // Type of the heightfield partitioning.
    NavmeshPartitionType partitionType_{NAVMESH_PARTITION_MONOTONE};
};

}