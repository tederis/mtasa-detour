#include "../scene/Scene.h"
#include "../scene/World.h"
#include "../navigation/NavigationMesh.h"
#include "../navigation/NavBuildData.h"
#include "../navigation/NavArea.h"

#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
#include <Recast.h>

#include <spdlog/spdlog.h>

namespace WorldAssistant
{

static const int MAX_POLYS = 2048;

// Temporary data for finding a path.
struct FindPathData
{
    // Polygons.
    dtPolyRef polys_[MAX_POLYS]{};
    // Polygons on the path.
    dtPolyRef pathPolys_[MAX_POLYS]{};
    // Points on the path.
    Vector3F pathPoints_[MAX_POLYS];
    // Flags on the path.
    unsigned char pathFlags_[MAX_POLYS]{};
};

NavigationMesh::NavigationMesh(World* world) noexcept :
    world_(world),
    padding_(1.0f, 1.0f, 1.0f),
    queryFilter_(new dtQueryFilter()),
    pathData_(new FindPathData())
{
}

NavigationMesh::~NavigationMesh()
{
    ReleaseNavigationMesh();

    delete queryFilter_;
    queryFilter_ = {};

    delete pathData_;
    pathData_ = {};
}

bool NavigationMesh::Allocate(const BoundingBox& boundingBox, unsigned maxTiles)
{
    assert(nullptr);
    return false;
}

bool NavigationMesh::Build()
{
    assert(nullptr);
    return false;
}

bool NavigationMesh::Build(const BoundingBox& boundingBox)
{
    assert(nullptr);
    return false;
}

bool NavigationMesh::Build(const Int32Vector2& from, const Int32Vector2& to)
{
    assert(nullptr);
    return false;
}

std::vector<unsigned char> NavigationMesh::GetTileData(const Int32Vector2& tile) const
{
    assert(nullptr);
    return std::vector<unsigned char>();
}

bool NavigationMesh::AddTile(const std::vector<unsigned char>& tileData)
{
    assert(nullptr);
    return false;
}

void NavigationMesh::RemoveTile(const Int32Vector2& tile)
{
    if (!navMesh_)
        return;

    const dtTileRef tileRef = navMesh_->getTileRefAt(tile.x_, tile.y_, 0);
    if (!tileRef)
        return;

    navMesh_->removeTile(tileRef, nullptr, nullptr);
}

void NavigationMesh::RemoveAllTiles()
{
    const dtNavMesh* navMesh = navMesh_;
    for (int i = 0; i < navMesh_->getMaxTiles(); ++i)
    {
        const dtMeshTile* tile = navMesh->getTile(i);
        assert(tile);
        if (tile->header)
            navMesh_->removeTile(navMesh_->getTileRef(tile), nullptr, nullptr);
    }
}

bool NavigationMesh::HasTile(const Int32Vector2& tile) const
{
    if (navMesh_)
        return !!navMesh_->getTileAt(tile.x_, tile.y_, 0);
    return false;
}

Vector3F NavigationMesh::FindNearestPoint(const Vector3F& point, const Vector3F& extents, const dtQueryFilter* filter, dtPolyRef* nearestRef)
{
    if (!InitializeQuery())
        return point;

    Vector3F localPoint = point;
    Vector3F nearestPoint;

    dtPolyRef pointRef;
    if (!nearestRef)
        nearestRef = &pointRef;
    navMeshQuery_->findNearestPoly(&localPoint.x_, &extents.x_, filter ? filter : queryFilter_, nearestRef, &nearestPoint.x_);
    return *nearestRef ? nearestPoint : point;
}

void NavigationMesh::FindPath(std::vector<Vector3F>& dest, const Vector3F& start, const Vector3F& end, const Vector3F& extents, const dtQueryFilter* filter)
{
    std::vector<NavigationPathPoint> navPathPoints;
    FindPath(navPathPoints, start, end, extents, filter);

    dest.clear();
    for (unsigned i = 0; i < navPathPoints.size(); ++i)
        dest.push_back(navPathPoints[i].position_);
}

void NavigationMesh::FindPath(std::vector<NavigationPathPoint>& dest, const Vector3F& start, const Vector3F& end, const Vector3F& extents, const dtQueryFilter* filter)
{
    Scene* scene = world_->GetScene();
    assert(scene);

    dest.clear();

    if (!InitializeQuery())
        return;

    Vector3F localStart = start;
    Vector3F localEnd = end;

    const dtQueryFilter* queryFilter = filter ? filter : queryFilter_;
    dtPolyRef startRef;
    dtPolyRef endRef;
    navMeshQuery_->findNearestPoly(&localStart.x_, &extents.x_, queryFilter, &startRef, nullptr);
    navMeshQuery_->findNearestPoly(&localEnd.x_, &extents.x_, queryFilter, &endRef, nullptr);

    if (!startRef || !endRef)
        return;

    int numPolys = 0;
    int numPathPoints = 0;

    navMeshQuery_->findPath(startRef, endRef, &localStart.x_, &localEnd.x_, queryFilter, pathData_->polys_, &numPolys,
        MAX_POLYS);
    if (!numPolys)
        return;

    Vector3F actualLocalEnd = localEnd;

    // If full path was not found, clamp end point to the end polygon
    if (pathData_->polys_[numPolys - 1] != endRef)
        navMeshQuery_->closestPointOnPoly(pathData_->polys_[numPolys - 1], &localEnd.x_, &actualLocalEnd.x_, nullptr);

    navMeshQuery_->findStraightPath(&localStart.x_, &actualLocalEnd.x_, pathData_->polys_, numPolys,
        &pathData_->pathPoints_[0].x_, pathData_->pathFlags_, pathData_->pathPolys_, &numPathPoints, MAX_POLYS);

    // Transform path result back to world space
    for (int i = 0; i < numPathPoints; ++i)
    {
        NavigationPathPoint pt;
        pt.position_ = pathData_->pathPoints_[i];
        pt.flag_ = (NavigationPathPointFlag)pathData_->pathFlags_[i];

        // Walk through all NavAreas and find nearest
        unsigned nearestNavAreaID = 0;       // 0 is the default nav area ID
        float nearestDistance = M_LARGE_VALUE;

        for (const auto& area : scene->GetNavAreas()) {
            if (area && area->IsEnabled())
            {
                BoundingBox bb = area->GetBoundingBox();
                if (bb.IsInside(pt.position_) == INSIDE)
                {
                    Vector3F areaWorldCenter = bb.Center();
                    float distance = (areaWorldCenter - pt.position_).LengthSquared();
                    if (distance < nearestDistance)
                    {
                        nearestDistance = distance;
                        nearestNavAreaID = area->GetAreaID();
                    }
                }
            }
        }
        pt.areaID_ = (unsigned char)nearestNavAreaID;

        dest.push_back(pt);
    }
}

BoundingBox NavigationMesh::GetTileBoundingBox(const Int32Vector2& tile) const
{
    const float tileEdgeLength = (float)tileSize_ * cellSize_;
    return BoundingBox(
        Vector3F(
            boundingBox_.min_.x_ + tileEdgeLength * (float)tile.x_,
            boundingBox_.min_.y_,
            boundingBox_.min_.z_ + tileEdgeLength * (float)tile.y_
        ),
        Vector3F(
            boundingBox_.min_.x_ + tileEdgeLength * (float)(tile.x_ + 1),
            boundingBox_.max_.y_,
            boundingBox_.min_.z_ + tileEdgeLength * (float)(tile.y_ + 1)
        ));
}

bool NavigationMesh::BuildTile(int x, int z)
{
    assert(nullptr);
    return false;
}

void NavigationMesh::GetTileGeometry(NavBuildData* build, BoundingBox& box)
{
    Scene* scene = world_->GetScene();
    assert(scene);

    std::vector<const SceneNode*> result;
	scene->Query(&box.min_.x_, result);

    for (const auto& node : result) {
        auto* collision = world_->GetModelCollision(node->GetModel());
		if (!collision || collision->Empty()) {
			spdlog::warn("Could not find a collision for model {}", node->GetModel());
			continue;
		}

		collision->Unpack(build->vertices_, build->indices_, node->GetTransform(), static_cast<std::int32_t>(build->vertices_.size()));
    }
}

bool NavigationMesh::InitializeQuery()
{
    if (!navMesh_)
        return false;

    if (navMeshQuery_)
        return true;

    navMeshQuery_ = dtAllocNavMeshQuery();
    if (!navMeshQuery_)
    {
        spdlog::error("Could not create navigation mesh query");
        return false;
    }

    if (dtStatusFailed(navMeshQuery_->init(navMesh_, MAX_POLYS)))
    {
        spdlog::error("Could not init navigation mesh query");
        return false;
    }

    return true;
}

void NavigationMesh::ReleaseNavigationMesh()
{
    dtFreeNavMesh(navMesh_);
    navMesh_ = nullptr;

    dtFreeNavMeshQuery(navMeshQuery_);
    navMeshQuery_ = nullptr;

    numTilesX_ = 0;
    numTilesZ_ = 0;
    boundingBox_.Clear();
}

}