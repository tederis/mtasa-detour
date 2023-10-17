#pragma once

#include <memory>
#include <filesystem>

#include "../navigation/DynamicNavigationMesh.h"
#include "../scene/World.h"
#include "../scene/Scene.h"

namespace WorldAssistant
{

class Navigation
{
public:
	static Navigation& GetInstance()
	{
		static Navigation instance;
		return instance;
	}

	Navigation(Navigation const&) = delete;

    void operator=(Navigation const&) = delete;

	bool Initialize();

	void Shutdown();

	bool Save(const std::filesystem::path& path);

	bool Load(const std::filesystem::path& path);

	bool Dump(const std::filesystem::path& path);

	World* GetWorld() const { return world_.get(); }

	DynamicNavigationMesh* GetNavMesh() const { return navmesh_.get(); }

private:
	Navigation()
	{
	}

	std::unique_ptr<World> world_;

	std::shared_ptr<DynamicNavigationMesh> navmesh_;
};

}