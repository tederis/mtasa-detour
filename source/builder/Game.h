#pragma once

#include <unordered_set>
#include <filesystem>
#include <array>

#include "../game/IMGArchive.h"
#include "../game/Collision.h"
#include "../scene/World.h"

namespace WorldAssistant
{

static constexpr uint32_t LOD_FLAG = 0x80000000;

struct GameDefinition
{
	std::string name_;
};

struct GamePlacement
{
	float x_{};
	float y_{};
	float z_{};
	float rx_{};
	float ry_{};
	float rz_{};
	float rw_{};
	uint32_t model_{};
	int32_t interior_{};
	uint32_t flags_{};
};
static_assert(sizeof(GamePlacement) == 40, "Invalid size of GamePlacement");

struct PlacementModifier
{
	const std::unordered_set<uint32_t>& ignoredModels_;
	const std::unordered_set<int32_t>& excludedInteriors_;
	bool excludeLODs_{ true };
};

class Game
{
public:
	Game(const std::filesystem::path& gamePath);

	bool Load();

	const std::vector<std::shared_ptr<IMGArchive>>& GetArchives() const { return archives_; }

	const std::vector<GamePlacement>& GetPlacements() const { return placements_; }

	const GameDefinition* GetDefinition(uint32_t model) const;

	std::size_t ApplyPlacementModifier(const PlacementModifier& modifier);

	std::size_t ApplyCollisionModifier(const CollisionModifier& modifier);

	std::shared_ptr<World> CreateWorld();

private:
	bool LoadGTA(const std::filesystem::path& filePath);

	bool LoadIMG(const std::filesystem::path& filePath);

	bool LoadIDE(const std::filesystem::path& filePath);

	bool LoadIPL(const std::filesystem::path& filePath);

	bool LoadIPLBinary(const std::vector<std::uint8_t>& buffer);

	void InsertPlacement(const GamePlacement& placement);

	std::filesystem::path gameDir_;

	std::vector<std::shared_ptr<IMGArchive>> archives_;

	std::unordered_map<uint32_t, GameDefinition> defs_;

	std::vector<GamePlacement> placements_;

	std::unique_ptr<CollisionFile> collisions_;
};

}