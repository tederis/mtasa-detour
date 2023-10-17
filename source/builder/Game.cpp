#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <memory>

#include <pugixml.hpp>

#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

#include <spdlog/spdlog.h>

#include "../builder/Game.h"
#include "../game/IMGArchive.h"
#include "../game/Collision.h"
#include "../utils/UtilsString.h"
#include "../utils/UtilsStream.h"

namespace WorldAssistant
{

Game::Game(const std::filesystem::path& gamePath) :
	gameDir_(gamePath)
{	
	collisions_ = std::make_unique<CollisionFile>();
}

bool Game::Load()
{
	std::filesystem::path gtaPath = gameDir_ / "data/gta.dat";
	if (!std::filesystem::exists(gtaPath)) {
		return false;
	}

	return LoadGTA(gtaPath);
}

const GameDefinition* Game::GetDefinition(uint32_t model) const
{
	if (auto found = defs_.find(model); found != defs_.end()) {
		return &found->second;
	}

	return {};
}

std::size_t Game::ApplyPlacementModifier(const PlacementModifier& modifier)
{
	const std::size_t count = std::erase_if(placements_, [&modifier](const GamePlacement& placement) {
		if (modifier.excludeLODs_ && placement.flags_ & LOD_FLAG) {
			//return true;
		}

		return modifier.ignoredModels_.contains(placement.model_) || modifier.excludedInteriors_.contains(placement.interior_);
	});

	return count;
}

std::size_t Game::ApplyCollisionModifier(const CollisionModifier& modifier)
{
	return collisions_->ApplyModifier(modifier);
}

std::shared_ptr<World> Game::CreateWorld()
{
	std::shared_ptr<World> world = std::make_shared<World>();

	// Insert collisions
	CollisionFile* cache = world->GetCollisionCache();
	cache->Insert(collisions_.get());

	// Insert defs
	for (const auto& [model, def] : defs_) {
		world->SetModelDesc(model, ModelDesc {
			.name_ = def.name_
		});
	}

	// Insert nodes
	Scene* scene = world->GetScene();
	for (const auto& placement : placements_) {
		glm::mat4 transform = glm::toMat4(
			glm::quat(placement.rw_, placement.rx_, placement.rz_, placement.ry_)
		);
		transform[3][0] = placement.x_;
		transform[3][1] = placement.z_;
		transform[3][2] = placement.y_;

		SceneNode* node = scene->AddNode(placement.model_, transform);
		if (node) {
			node->SetInterior(placement.interior_);
			node->SetFlags(placement.flags_);
		}
	}	

	return world;
}

bool Game::LoadGTA(const std::filesystem::path& filePath)
{
	std::ifstream file(filePath);
	if (!file.is_open()) {
		return false;
	}	

	std::string line;

	// IMGs
	while (std::getline(file, line))
	{
		// Trim left
		line.erase(0, line.find_first_not_of(" "));

		if (line.starts_with("IMG")) {
			LoadIMG(line.substr(4));
		}
		else if (line.starts_with("IDE")) {
			LoadIDE(line.substr(4));
		}
		else if (line.starts_with("IPL")) {
			LoadIPL(line.substr(4));
		}
	}

	// Load additional resources
	LoadIMG("models/gta3.img");
	LoadIMG("models/gta_int.img");
	LoadIMG("models/player.img");

	std::vector<std::uint8_t> buffer;

	CollisionFile collision;

	for (const auto& archive : archives_) {
		// Load IPL
		archive->ForEach(".*\\.ipl$", [this, &filePath, &buffer, archive = archive.get()](const IMGEntry& entry) {
			if (archive->Read(buffer, entry.offset_, entry.streamingSize_)) {
				if (LoadIPLBinary(buffer)) {
					spdlog::info("Binary IPL {} was successfully loaded", entry.name_);
				}
			}
		});

		// Load COL
		archive->ForEach(".*\\.col$", [this, &archive, &buffer, &collision](const IMGEntry& entry) {
			if (!archive->Read(buffer, entry.offset_, entry.streamingSize_)) {
				return;
			}

            InputMemoryStream stream(buffer);
            if (collision.Load(stream)) {
				collisions_->Insert(&collision);
			}
		});
	}

	spdlog::info("GTA assets loaded: {} collisions, {} nodes, {} defs", collisions_->GetCollisionsNum(), placements_.size(), defs_.size());

	return true;
}

bool Game::LoadIMG(const std::filesystem::path& filePath)
{
	std::shared_ptr<IMGArchive> archive = std::make_shared<IMGArchive>();
	if (archive->Open(gameDir_ / filePath)) {		
		archives_.push_back(std::move(archive));
		spdlog::info("IMG archive {} was successfully loaded", filePath.string());

		return true;
	}

	return false;
}

bool Game::LoadIDE(const std::filesystem::path& filePath)
{
	std::ifstream file(gameDir_ / filePath);
	if (!file.is_open()) {
		return false;
	}

	enum class SectionType
	{
		None = 0,
		Objs,
		Anim,
		TObj
	};

	SectionType insideSection{SectionType::None};
	std::string line;

	// IMGs
	while (std::getline(file, line))
	{
		// Trim left
		line.erase(0, line.find_first_not_of(" "));

		if (line.starts_with("objs")) {
			insideSection = SectionType::Objs;
		}
		else if (line.starts_with("anim")) {
			insideSection = SectionType::Anim;
		}
		else if (line.starts_with("tobj")) {
			insideSection = SectionType::TObj;
		}
		else if (line.starts_with("end")) {
			insideSection = SectionType::None;
		}
		else if (insideSection != SectionType::None) {
			auto args = Split(line, ',', true);

			if (args.size() >= 2)	{		
				unsigned int model = static_cast<unsigned int>(std::stoul(args[0]));

				GameDefinition def{ args[1] };
				def.name_ = Trim(def.name_);

				defs_[model] = def;			
			}	
		}
	}

	spdlog::info("IDE {} was successfully loaded", filePath.string());

	return true;
}

bool Game::LoadIPL(const std::filesystem::path& filePath)
{
	std::ifstream file(gameDir_ / filePath);
	if (!file.is_open()) {
		return false;
	}

	bool insideSection = false;
	std::string line;

	std::vector<GamePlacement> placements;
	std::unordered_set<uint32_t> lods;

	// IMGs
	while (std::getline(file, line))
	{
		// Trim left
		line.erase(0, line.find_first_not_of(" "));

		if (line.starts_with("inst")) {
			insideSection = true;
		}
		else if (line.starts_with("end")) {
			insideSection = false;
		}
		else if (insideSection) {
			auto args = Split(line, ',', true);
			if (args.size() >= 10) {
				const std::uint32_t model = static_cast<std::uint32_t>(std::stoul(args[0]));
				const std::int32_t interior = std::stoi(args[2]);
				const float posX = std::stof(args[3]);
				const float posY = std::stof(args[4]);
				const float posZ = std::stof(args[5]);
				const float rotX = std::stof(args[6]);
				const float rotY = std::stof(args[7]);
				const float rotZ = std::stof(args[8]);
				const float rotW = std::stof(args[9]);		

				placements.emplace_back(GamePlacement{posX, posY, posZ, rotX, rotY, rotZ, rotW, model, interior, 0u});

				if (interior >= 0) {
					lods.insert(interior);
				}		
			}
		}
	}

	for (size_t i = 0; auto& placement : placements) {
		const auto* definition = GetDefinition(placement.model_);
		if (definition) {
			const bool isLOD = lods.contains(i) || definition->name_.starts_with("LOD");
			if (isLOD) {
				placement.flags_ |= LOD_FLAG;
			}
			else {
				placement.flags_ &= ~LOD_FLAG;
			}

			InsertPlacement(placement);	
		}
		else {
			spdlog::error("Could not find a definition for the model {}", placement.model_);
		}

		++i;
	}

	spdlog::info("IPL {} was successfully loaded", filePath.string());

	return true;
}

bool Game::LoadIPLBinary(const std::vector<std::uint8_t>& buffer)
{
	// Parse buffer
	InputMemoryStream stream(buffer);

	const std::string fileID = stream.ReadFileID();
	if (fileID != "bnry") {
		return false;
	}

	const unsigned int instanciesNum = stream.ReadUInt();
	const unsigned int unknownNum1 = stream.ReadUInt();
	const unsigned int unknownNum2 = stream.ReadUInt();
	const unsigned int unknownNum3 = stream.ReadUInt();
	const unsigned int carsNum = stream.ReadUInt();
	const unsigned int unknownNum4 = stream.ReadUInt();
	const unsigned int instanciesOffset = stream.ReadUInt();

	if (instanciesOffset != 76) {
		return false;
	}

	stream.Seek(instanciesOffset);

	std::vector<GamePlacement> placements;
	std::unordered_set<uint32_t> lods;

	for (unsigned i = 0; i < instanciesNum; ++i) {
		GamePlacement def;
		stream.Read(&def, sizeof(GamePlacement));
	
		placements.push_back(def);

		if (def.interior_ >= 0) {
			lods.insert(def.interior_);
		}
	}

	for (size_t i = 0; auto& placement : placements) {
		const auto* definition = GetDefinition(placement.model_);
		if (definition) {
			const bool isLOD = lods.contains(i) || definition->name_.starts_with("LOD");
			if (isLOD) {
				placement.flags_ |= LOD_FLAG;
			}
			else {
				placement.flags_ &= ~LOD_FLAG;
			}

			InsertPlacement(placement);	
		}
		else {
			spdlog::error("Could not find a definition for the model {}", placement.model_);
		}

		++i;
	}

	return true;
}

void Game::InsertPlacement(const GamePlacement& placement)
{
	placements_.push_back(placement);
}

}