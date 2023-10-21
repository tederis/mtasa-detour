#include <cstring>

#include "../module/Module.h"
#include "../module/Navigation.h"

#ifdef EXPORT_LUA_API

using namespace WorldAssistant;

ILuaModuleManager10* pModuleManager = NULL;

// Initialisation function (module entrypoint)
MTAEXPORT bool InitModule(ILuaModuleManager10* pManager, char* szModuleName, char* szAuthor, float* fVersion)
{
    pModuleManager = pManager;

    // Set the module info
    strncpy(szModuleName, MODULE_NAME, MAX_INFO_LENGTH);
    strncpy(szAuthor, MODULE_AUTHOR, MAX_INFO_LENGTH);
    (*fVersion) = MODULE_VERSION;

    Navigation::GetInstance().Initialize();

    return true;
}

MTAEXPORT void RegisterFunctions(lua_State* luaVM)
{
    if (pModuleManager && luaVM)
    {
        pModuleManager->RegisterFunction(luaVM, "navState", LuaBinding::navState);
        pModuleManager->RegisterFunction(luaVM, "navLoad", LuaBinding::navLoad);
        pModuleManager->RegisterFunction(luaVM, "navSave", LuaBinding::navSave);
        pModuleManager->RegisterFunction(luaVM, "navFindPath", LuaBinding::navFindPath);
        pModuleManager->RegisterFunction(luaVM, "navNearestPoint", LuaBinding::navNearestPoint);
        pModuleManager->RegisterFunction(luaVM, "navDump", LuaBinding::navDump);
        pModuleManager->RegisterFunction(luaVM, "navBuild", LuaBinding::navBuild);
        pModuleManager->RegisterFunction(luaVM, "navCollisionMesh", LuaBinding::navCollisionMesh);
        pModuleManager->RegisterFunction(luaVM, "navNavigationMesh", LuaBinding::navNavigationMesh);
        pModuleManager->RegisterFunction(luaVM, "navScanWorld", LuaBinding::navScanWorld);
    }
}

MTAEXPORT bool DoPulse(void)
{
    return true;
}

MTAEXPORT bool ShutdownModule(void)
{
    Navigation::GetInstance().Shutdown();
    return true;
}

MTAEXPORT bool ResourceStopping(lua_State* luaVM)
{
    return true;
}

MTAEXPORT bool ResourceStopped(lua_State* luaVM)
{
    return true;
}

#endif // EXPORT_LUA_API