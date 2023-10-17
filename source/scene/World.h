#pragma once

#include <memory>
#include <filesystem>
#include <unordered_map>

#include "../scene/Scene.h"
#include "../game/Collision.h"

namespace WorldAssistant
{

struct ModelDesc
{
	std::string name_;
};

struct WorldLoadDesc
{
	WorldLoadDesc() = default;

	WorldLoadDesc(const std::filesystem::path& basePath);

	void ConstructPaths(const std::filesystem::path& basePath);

	std::filesystem::path defsPath_;

	std::filesystem::path nodesPath_;

	std::filesystem::path collisionsPath_;
};

class World
{
public:
	World();

	bool Load(const WorldLoadDesc& desc);

	bool Save(const WorldLoadDesc& desc);

	void SetModelDesc(uint32_t model, ModelDesc&& desc);

	const ModelDesc* GetModelDesc(uint32_t model) const;

	const Collision* GetModelCollision(uint32_t model) const;

	Scene* GetScene() const { return scene_.get(); }

	CollisionFile* GetCollisionCache() const { return cache_.get(); }

private:
	bool LoadDefinitions(const std::filesystem::path& path);

	bool LoadPlacements(const std::filesystem::path& path);

	bool LoadCollisionCache(const std::filesystem::path& path);

	bool ExportDefinitions(const std::filesystem::path& path);

	std::unordered_map<uint32_t, ModelDesc> models_;

	std::shared_ptr<Scene> scene_;

	std::shared_ptr<CollisionFile> cache_;
};

}