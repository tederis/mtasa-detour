#include <iostream>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#include "../builder/Application.h"
#include "../game/Collision.h"

namespace WorldAssistant
{

namespace
{
    const std::unordered_set<uint8_t> IGNORED_MATERIALS = {
        96, 97, 98, 99,
        100, 155, 156, 
        154,
        38, 39,

        48, 49, // Empty

        158 // Door
    };

    const std::unordered_set<uint32_t> IGNORED_MODELS = {
		// sfse_roadblock*
		4523, 4524, 4525, 4526, 4527,
		// sfw_roadblock*ld
		4510, 4511, 4512,
		// CE_Fredbarld
		4518, 4519, 4520,

		4514, 4515, 4516, 4517,

		4504, 4505, 4506, 4507, 4508, 4509
	};

    const std::unordered_set<int32_t> EXCLUDED_INTERIORS = { 
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17, 18 
    };
}

/*
    Application
*/
Application::Application(const ApplicationParameters& params) noexcept :
    params_(params)
{
}

bool Application::Run()
{
    try
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
        console_sink->set_pattern("%v");

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/log.txt", true);

        auto logger = std::make_shared<spdlog::logger>("logger");
        logger->sinks().push_back(console_sink);
        logger->sinks().push_back(file_sink);

        spdlog::set_default_logger(logger);
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cout << "Log init failed: " << ex.what() << std::endl;
    }
    
    // Load and process game
    {
        spdlog::info("Start GTA assets loading...");

        game_ = std::make_unique<Game>(params_.gamePath_);
        if (!game_->Load())
        {
            spdlog::error("Cound not load a game");
            return false;
        }    

        // Remove specific models
        const PlacementModifier placementModifier = {
            .ignoredModels_ = IGNORED_MODELS,
            .excludedInteriors_ = EXCLUDED_INTERIORS
        };
        const size_t nodesRemoved = game_->ApplyPlacementModifier(placementModifier);
        spdlog::info("{} nodes removed", nodesRemoved);

        // Remove triangles with specific materials
        const CollisionModifier collisionModifier {
            .ignoredMaterials_ = IGNORED_MATERIALS
        };
        const size_t trisRemoved = game_->ApplyCollisionModifier(collisionModifier);       
        spdlog::info("{} triangles removed", trisRemoved);
    }

    auto world = game_->CreateWorld();
    if (world) {
        spdlog::info("Start assets saving...");

        return world->Save(WorldLoadDesc(params_.outputPath_));
    }

    return false;
}

}