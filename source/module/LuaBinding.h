#pragma once

#include <stdio.h>
#include "module-sdk/ILuaModuleManager.h"

extern ILuaModuleManager10* pModuleManager;

namespace WorldAssistant
{

class LuaBinding
{
public:
    static int navLoad(lua_State* luaVM);
    static int navFindPath(lua_State* luaVM);
    static int navNearestPoint(lua_State* luaVM);
    static int navDump(lua_State* luaVM);
    static int navBuild(lua_State* luaVM);
    static int navCollisionMesh(lua_State* luaVM);
    static int navNavigationMesh(lua_State* luaVM);
    static int navScanWorld(lua_State* luaVM);
};

}