#include "../scene/World.h"

#include <pugixml.hpp>
#include <spdlog/spdlog.h>

namespace WorldAssistant
{

/*
    WorldLoadDesc
*/
WorldLoadDesc::WorldLoadDesc(const std::filesystem::path& basePath)
{
    ConstructPaths(basePath);
}

void WorldLoadDesc::ConstructPaths(const std::filesystem::path& basePath)
{
    defsPath_ = basePath / "defs.xml";
    nodesPath_ = basePath / "nodes.xml";
    collisionsPath_ = basePath / "cols.col";
}

/*
    World
*/
World::World()
{
    cache_ = std::make_shared<CollisionFile>();
	scene_ = std::make_shared<Scene>(this);
}

bool World::Load(const WorldLoadDesc& desc)
{
    if (!LoadDefinitions(desc.defsPath_)) {
        return false;
    }

    if (!LoadCollisionCache(desc.collisionsPath_)) {
        return false;
    }

    if (!LoadPlacements(desc.nodesPath_)) {
        return false;
    }    

    return true;
}

bool World::Save(const WorldLoadDesc& desc)
{
    if (!ExportDefinitions(desc.defsPath_)) {
        return false;
    }

    if (!scene_->Save(desc.nodesPath_)) {
        return false;
    }

    if (!cache_->Save(desc.collisionsPath_)) {
        return false;
    }

    return true;
}

bool World::LoadDefinitions(const std::filesystem::path& path)
{
    pugi::xml_document doc;

    pugi::xml_parse_result result = doc.load_file(path.c_str());
    if (!result) {
        spdlog::error("Invalid defs file");
        return false;
    }

    const auto root = doc.child("defs");
    if (!root) {
        spdlog::error("Invalid defs file");
        return false;
    }

    for (pugi::xml_node tool = root.child("def"); tool; tool = tool.next_sibling("def"))
    {
        const uint32_t model = tool.attribute("model").as_uint();

        ModelDesc modelDesc;
        modelDesc.name_ = tool.attribute("name").as_string();

        models_[model] = modelDesc;
    }

    return true;
}

bool World::LoadPlacements(const std::filesystem::path& path)
{
    if (!scene_->Load(path)) {
		spdlog::error("Invalid scene file");
		return false;
	}

    return true;
}

bool World::LoadCollisionCache(const std::filesystem::path& path)
{
    if (!cache_->Load(path)) {
		spdlog::error("Invalid collision cache file");
		return false;
	}

    return true;
}

bool World::ExportDefinitions(const std::filesystem::path& path)
{
    const auto parentPath = path.parent_path();

	// Create directory if not exists
	if (!std::filesystem::exists(parentPath)) {
		std::filesystem::create_directories(parentPath);
	}

	pugi::xml_document doc;
	pugi::xml_node node = doc.append_child("defs");

    for (const auto& [model, def] : models_) {
        pugi::xml_node entryNode = node.append_child("def");
		entryNode.append_attribute("model") = model;
		entryNode.append_attribute("name") = def.name_.c_str();
	
    }

    std::ofstream stream(path);
	doc.save(stream);

    return true;
}

void World::SetModelDesc(uint32_t model, ModelDesc&& desc)
{
    models_[model] = std::move(desc);
}

const ModelDesc* World::GetModelDesc(uint32_t model) const
{
    if (auto found = models_.find(model); found != models_.end())
        return &found->second;

    return {};
}

const Collision* World::GetModelCollision(uint32_t model) const
{
    const auto* desc = GetModelDesc(model);
    if (!desc) {
        return {};
    }

    return cache_->GetCollision(desc->name_);
}

}