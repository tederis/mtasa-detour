#include "Navigation.h"
#include "../utils/DebugMesh.h"
#include "../module/LuaBinding.h"
#include "../scene/Scene.h"
#include "../navigation/DynamicNavigationMesh.h"

#include <fstream>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
 
extern ILuaModuleManager10* pModuleManager;

namespace spdlog
{
namespace sinks
{

template<typename Mutex>
class server_sink : public spdlog::sinks::base_sink<Mutex>
{
protected:
    void sink_it_(const spdlog::details::log_msg &msg) override
    {
        std::vector<char> payload(msg.payload.size() + 1);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
        std::memcpy(payload.data(), msg.payload.data(), msg.payload.size());
        payload[msg.payload.size()] = 0;

        pModuleManager->Printf("%s\n", payload.data());
    }

    void flush_() override
    {
    }

    void set_pattern_(const std::string &pattern) override
    {
    }

    void set_formatter_(std::unique_ptr<spdlog::formatter> sink_formatter) override
    {
    }
};

using server_sink_mt = server_sink<std::mutex>;

}
}

namespace WorldAssistant
{

bool Navigation::Initialize()
{
	try
    {
        auto sink = std::make_shared<spdlog::sinks::server_sink_mt>();
        auto logger = std::make_shared<spdlog::logger>("server_logger", sink);

        spdlog::set_default_logger(logger);
    }
    catch ([[maybe_unused]] const spdlog::spdlog_ex& ex)
    {
        return false;
    }  

	world_ = std::make_unique<World>();
    if (!world_->Load(WorldLoadDesc("navmesh"))) {
        return false;
    }

    navmesh_ = std::make_shared<DynamicNavigationMesh>(world_.get());   

	spdlog::info("Navigation module successfully loaded");

    return true;
}

void Navigation::Shutdown()
{
	navmesh_.reset();
	world_.reset();

    spdlog::set_default_logger({});
    spdlog::shutdown();
}

bool Navigation::Save(const std::filesystem::path& path)
{
    if (!navmesh_) {
        return false;
    }

    const auto cached = navmesh_->Serialize();
	if (cached.size() > 0) {
		std::ofstream stream(path, std::ios::out | std::ios::binary);
		if (stream.is_open()) {
			stream.write(reinterpret_cast<const char*>(cached.data()), cached.size());
            return true;
		}
	}

    return false;
}

bool Navigation::Load(const std::filesystem::path& path)
{
    if (!navmesh_) {
        return false;
    }

    std::ifstream stream(path, std::ios::out | std::ios::binary | std::ios::ate);
	if (stream.is_open()) {
		const auto size = stream.tellg();
		stream.seekg(0);

		std::vector<unsigned char> buffer;
		buffer.resize(static_cast<std::size_t>(size));

		stream.read(reinterpret_cast<char*>(buffer.data()), size);

		if (buffer.size() > 0) {
			navmesh_->Deserialize(buffer);
            return true;
		}
	}

    return false;
}

bool Navigation::Dump(const std::filesystem::path& path)
{
    if (navmesh_) {
        DebugMesh mesh;
        navmesh_->Dump(mesh);
        mesh.Dump(path);
        return true;
    }

    return false;
}

}