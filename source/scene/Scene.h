#pragma once

#include <filesystem>

#include <glm/glm.hpp>

#include "../navigation/Obstacle.h"
#include "../navigation/OffMeshConnection.h"
#include "../navigation/NavArea.h"
#include "../utils/Quadtree.h"
#include "../utils/MathUtils.h"
#include "../utils/UtilsContainer.h"

namespace WorldAssistant
{

class World;

class SceneNode : public QuadtreeValue, public LinkedListNode
{
	friend class Scene;
public:
	void SetModel(uint32_t model) { model_ = model; }

	void SetInterior(int32_t interior) { interior_ = interior; }

	void SetFlags(uint32_t flags) { flags_ = flags; }

	void SetPosition(const glm::vec3& pos);

	void SetRotation(const glm::vec3& rot);

	void SetRotation(const glm::quat& rot);

	const glm::mat4& GetTransform() const { return transform_; }

	uint32_t GetModel() const { return model_; }

	int32_t GetInterior() const { return interior_; }

	uint32_t GetFlags() const { return flags_; }

private:
	int32_t interior_{};

	uint32_t flags_{};

	uint32_t model_{};

	glm::mat4 transform_;

	bool dirty_{ true };
};

class Scene
{
public:
	Scene(World* world);

	bool Load(const std::filesystem::path& path);

	bool Save(const std::filesystem::path& filename);

	SceneNode* AddNode(uint32_t model, const glm::mat4& transform = {});

	void RemoveNode(SceneNode* node);

	void Query(float* bounds, std::vector<const SceneNode*>& result);

	bool Empty() const;

	const std::vector<std::shared_ptr<Obstacle>>& GetObstacles() const { return obstacles_; }

	const std::vector<std::shared_ptr<OffMeshConnection>>& GetOffMeshConnections() const { return offMeshConnections_; }

	const std::vector<std::shared_ptr<NavArea>>& GetNavAreas() const { return navAreas_; }

	const BoundingBox& GetBounds() const { return bounds_; }

private:
	World* owner_{};

	Quadtree tree_;

	LinkedList<SceneNode> nodes_;

	std::vector<std::shared_ptr<Obstacle>> obstacles_;

	std::vector<std::shared_ptr<OffMeshConnection>> offMeshConnections_;

	std::vector<std::shared_ptr<NavArea>> navAreas_;

	BoundingBox bounds_;
};

}