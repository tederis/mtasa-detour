#include "../utils/DebugMesh.h"
#include "../module/Navigation.h"
#include "../module/LuaBinding.h"
#include "../scene/Scene.h"
#include "../navigation/DynamicNavigationMesh.h"

#include <fstream>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#ifdef EXPORT_LUA_API 
extern ILuaModuleManager10* pModuleManager;
#endif // EXPORT_LUA_API

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

#ifdef EXPORT_LUA_API 
        if (pModuleManager)
            pModuleManager->Printf("%s\n", payload.data());
#endif
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

    std::ofstream stream(path, std::ios::out | std::ios::binary);
    if (stream.is_open()) {
        OutputFileStream output(stream);
	    return navmesh_->Serialize(output);
    }

    return false;
}

bool Navigation::Load(const std::filesystem::path& path)
{
    if (!navmesh_) {
        return false;
    }

    std::ifstream stream(path, std::ios::in | std::ios::binary);
	if (stream.is_open()) {
        InputFileStream input(stream);
        return navmesh_->Deserialize(input);
    }	

    return false;
}

bool Navigation::Dump(const std::filesystem::path& path)
{
    if (!navmesh_) {
        return false;
    }

    DebugMesh mesh;
    if (navmesh_->Dump(mesh)) {
        mesh.Dump(path);
        return true;
    }

    return false;
}

}