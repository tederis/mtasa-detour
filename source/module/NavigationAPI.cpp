#include "../module/NavigationAPI.h"
#include "../module/Navigation.h"
#include "../utils/DebugMesh.h"
#include "../navigation/DynamicNavigationMesh.h"

#include <spdlog/spdlog.h>

#ifdef EXPORT_NATIVE_API

using namespace WorldAssistant;

namespace
{

struct NavigationArguments
{
    Vector3F pointStart_;

	Vector3F pointEnd_;

    Vector3F extents_;

    bool IsSame(const NavigationArguments& rhs) const
    {
        return pointStart_.Equals(rhs.pointStart_) && pointEnd_.Equals(rhs.pointEnd_) && extents_.Equals(rhs.extents_);
    }
};

template <class T>
struct NavigationCache
{
    explicit NavigationCache(std::vector<T>& arr) :
        array_(arr)
    {
    }

    std::vector<T>& array_;

    NavigationArguments args_;

    bool IsUpdateRequired(const NavigationArguments& rhs) const
    {
        return array_.empty() || !args_.IsSame(rhs);
    }
};

std::vector<Vector3F> SHARED_VECTORS;
std::vector<std::uint32_t> SHARED_NUMBERS;

NavigationCache<Vector3F> NAVMESH_PATH_CACHE(SHARED_VECTORS);
NavigationCache<Vector3F> COLLISION_VERTICES_CACHE(SHARED_VECTORS);
NavigationCache<Vector3F> NAVMESH_VERTICES_CACHE(SHARED_VECTORS);
NavigationCache<std::uint32_t> MODEL_INDICES_CACHE(SHARED_NUMBERS);

}

extern "C"
{

bool NAVIGATION_API navInit()
{
    return Navigation::GetInstance().Initialize();
}

void NAVIGATION_API navShutdown()
{
    Navigation::GetInstance().Shutdown();
}

bool NAVIGATION_API navState()
{
    auto& navigation = Navigation::GetInstance();
    if (auto* navMesh = navigation.GetNavMesh()) {
        return navMesh->GetEffectiveTilesCount() > 0u; 
    }

    return false;
}

bool NAVIGATION_API navLoad(const char* filename)
{
    auto& navigation = Navigation::GetInstance();    
    return navigation.Load(filename);
}

bool NAVIGATION_API navSave(const char* filename)
{
    auto& navigation = Navigation::GetInstance();    
    return navigation.Save(filename);
}

bool NAVIGATION_API navFindPath(float* startPos, float* endPos, std::uint32_t* outPointsNum, float* outPoints)
{
    if (outPointsNum == nullptr) {
        spdlog::error("Invalid points pointer");
        return false;
    }

    auto& navigation = Navigation::GetInstance();
    auto* navmesh = navigation.GetNavMesh();
    if (!navmesh) {
        return false;
    }  

    NavigationArguments args = {
        .pointStart_{ startPos },
        .pointEnd_{ endPos },
        .extents_ = { 2.0f, 2.0f, 2.0f },
    };

    std::swap(args.pointStart_.y_, args.pointStart_.z_);
    std::swap(args.pointEnd_.y_, args.pointEnd_.z_);

    // Update path if required
    if (NAVMESH_PATH_CACHE.IsUpdateRequired(args)) {
        navmesh->FindPath(NAVMESH_PATH_CACHE.array_, args.pointStart_, args.pointEnd_, args.extents_);

        for (auto& point : NAVMESH_PATH_CACHE.array_) {
            std::swap(point.y_, point.z_);
        }

        NAVMESH_PATH_CACHE.args_ = args;
    }    

    if (outPoints) {
        *outPointsNum = std::min(*outPointsNum, static_cast<std::uint32_t>(NAVMESH_PATH_CACHE.array_.size()));
        if (*outPointsNum == 0) {
            return false;
        }     

        std::memcpy(outPoints, NAVMESH_PATH_CACHE.array_.data(), static_cast<std::size_t>(*outPointsNum) * sizeof(Vector3F));
    }
    else {
        *outPointsNum = static_cast<std::uint32_t>(NAVMESH_PATH_CACHE.array_.size());
    }
    
    return true;
}

bool NAVIGATION_API navNearestPoint(float* pos, float* outPoint)
{
    auto& navigation = Navigation::GetInstance();
    auto* navmesh = navigation.GetNavMesh();
    if (!navmesh) {
        return false;
    }

    Vector3F point(pos);
	Vector3F extents(2.0f, 2.0f, 2.0f);

    dtPolyRef nearestRef{};

    Vector3F result = navmesh->FindNearestPoint(point, extents, nullptr, &nearestRef);      
      
    if (nearestRef) {
        std::memcpy(outPoint, &result.x_, sizeof(Vector3F));
        return true;
    }

    return false;
}

bool NAVIGATION_API navDump(const char* filename)
{
    auto& navigation = Navigation::GetInstance();
    return navigation.Dump(filename);
}

bool NAVIGATION_API navBuild()
{
    auto& navigation = Navigation::GetInstance(); 
    auto* navmesh = navigation.GetNavMesh();
    if (navmesh) {
        return navmesh->Build();
    }

    return false;
}

bool NAVIGATION_API navCollisionMesh(float* boundsMin, float* boundsMax, float bias, std::uint32_t* outVerticesNum, float* outVertices)
{
    if (outVerticesNum == nullptr) {
        spdlog::error("Invalid vertices pointer");
        return false;
    }

    auto& navigation = Navigation::GetInstance();
    World* world = navigation.GetWorld();
    Scene* scene = world->GetScene();

    NavigationArguments args = {
        .pointStart_{ boundsMin },
        .pointEnd_{ boundsMax }
    };

    std::swap(args.pointStart_.y_, args.pointStart_.z_);
    std::swap(args.pointEnd_.y_, args.pointEnd_.z_);

    // Update path if required
    if (COLLISION_VERTICES_CACHE.IsUpdateRequired(args)) {
        BoundingBox bounds;
        bounds.Merge(args.pointStart_);
        bounds.Merge(args.pointEnd_);

        std::vector<const SceneNode*> result;
	    scene->Query(&bounds.min_.x_, result);

        std::vector<Vector3F> vertices;
        std::vector<std::int32_t> indices;

        for (const auto& node : result) {
            auto* collision = world->GetModelCollision(node->GetModel());
		    if (!collision || collision->Empty()) {
			    continue;
		    }

		    collision->Unpack(vertices, indices, node->GetTransform(), static_cast<std::int32_t>(vertices.size()));
        }

        DebugMesh mesh(std::move(vertices), std::move(indices));
        for (auto& vertex : mesh.GetTriangleList()) {
            std::swap(vertex.y_, vertex.z_);
            vertex.z_ += bias;
        }

        COLLISION_VERTICES_CACHE.array_ = mesh.GetTriangleList();
        COLLISION_VERTICES_CACHE.args_ = args;
    }

    if (outVertices) {
        *outVerticesNum = std::min(*outVerticesNum, static_cast<std::uint32_t>(COLLISION_VERTICES_CACHE.array_.size()));
        if (*outVerticesNum == 0) {
            return false;
        }

        std::memcpy(outVertices, COLLISION_VERTICES_CACHE.array_.data(), static_cast<std::size_t>(*outVerticesNum) * sizeof(Vector3F));
    }    
    else {
        *outVerticesNum = static_cast<std::uint32_t>(COLLISION_VERTICES_CACHE.array_.size());
    }

    return true;
}

bool NAVIGATION_API navNavigationMesh(float* boundsMin, float* boundsMax, float bias, std::uint32_t* outVerticesNum, float* outVertices)
{
    if (outVerticesNum == nullptr) {
        spdlog::error("Invalid vertices pointer");
        return false;
    }

    auto* navmesh = Navigation::GetInstance().GetNavMesh();
    if (!navmesh) {
        return false;
    }

    NavigationArguments args = {
        .pointStart_{ boundsMin },
        .pointEnd_{ boundsMax }
    };

    std::swap(args.pointStart_.y_, args.pointStart_.z_);
    std::swap(args.pointEnd_.y_, args.pointEnd_.z_);

    // Update path if required
    if (NAVMESH_VERTICES_CACHE.IsUpdateRequired(args)) {
        BoundingBox bounds;
        bounds.Merge(args.pointStart_);
        bounds.Merge(args.pointEnd_);

        DebugMesh mesh;
        navmesh->Dump(mesh, true, &bounds);

        for (auto& vertex : mesh.GetTriangleList()) {
            std::swap(vertex.y_, vertex.z_);
            vertex.z_ += bias;
        }

        NAVMESH_VERTICES_CACHE.array_ = mesh.GetTriangleList();
        NAVMESH_VERTICES_CACHE.args_ = args;
    }

    if (outVertices) {
        *outVerticesNum = std::min(*outVerticesNum, static_cast<std::uint32_t>(NAVMESH_VERTICES_CACHE.array_.size()));
        if (*outVerticesNum == 0) {
            return false;
        }

        std::memcpy(outVertices, NAVMESH_VERTICES_CACHE.array_.data(), static_cast<std::size_t>(*outVerticesNum) * sizeof(Vector3F));
    }    
    else {
        *outVerticesNum = static_cast<std::uint32_t>(NAVMESH_VERTICES_CACHE.array_.size());
    }

    return true;
}

bool NAVIGATION_API navScanWorld(float* boundsMin, float* boundsMax, std::uint32_t* outModelsNum, std::uint32_t* outModels)
{
    if (outModelsNum == nullptr) {
        spdlog::error("Invalid vertices pointer");
        return false;
    }

    auto& navigation = Navigation::GetInstance();
    World* world = navigation.GetWorld();
    Scene* scene = world->GetScene();

    NavigationArguments args = {
        .pointStart_{ boundsMin },
        .pointEnd_{ boundsMax }
    };

    std::swap(args.pointStart_.y_, args.pointStart_.z_);
    std::swap(args.pointEnd_.y_, args.pointEnd_.z_);

    // Update path if required
    if (MODEL_INDICES_CACHE.IsUpdateRequired(args)) {
        BoundingBox bounds;
        bounds.Merge(args.pointStart_);
        bounds.Merge(args.pointEnd_);

        std::vector<const SceneNode*> result;
	    scene->Query(&bounds.min_.x_, result);

        MODEL_INDICES_CACHE.array_.clear();
        for (const auto node : result) {
            MODEL_INDICES_CACHE.array_.push_back(node->GetModel());
        }

        MODEL_INDICES_CACHE.args_ = args;
    }

    if (outModels) {
        *outModelsNum = std::min(*outModelsNum, static_cast<std::uint32_t>(MODEL_INDICES_CACHE.array_.size()));
        if (*outModelsNum == 0) {
            return false;
        }

        std::memcpy(outModels, MODEL_INDICES_CACHE.array_.data(), static_cast<std::size_t>(*outModelsNum) * sizeof(std::size_t));
    }    
    else {
        *outModelsNum = static_cast<std::uint32_t>(MODEL_INDICES_CACHE.array_.size());
    }

    return true;
}


}

#endif // EXPORT_NATIVE_API