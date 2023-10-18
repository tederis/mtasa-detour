#include "module-sdk/extra/CLuaArguments.h"

#include "../module/LuaBinding.h"
#include "../module/Navigation.h"
#include "../utils/DebugMesh.h"
#include "../navigation/DynamicNavigationMesh.h"

#pragma warning( push )
#pragma warning( disable : 4244 )

namespace WorldAssistant
{

int LuaBinding::navState(lua_State* luaVM)
{
    bool state{};

    auto& navigation = Navigation::GetInstance();
    if (auto* navMesh = navigation.GetNavMesh()) {
        state = navMesh->GetEffectiveTilesCount() > 0u; 
    }

    lua_pushboolean(luaVM, state);
    return 1;
}

int LuaBinding::navLoad(lua_State* luaVM)
{
    if (lua_type(luaVM, 1) != LUA_TSTRING) {
        lua_pushboolean(luaVM, false);
        return 1;
    }

    auto& navigation = Navigation::GetInstance();
    
    const char* path = lua_tostring(luaVM, 1);
    const bool result = navigation.Load(path);

    lua_pushboolean(luaVM, result);
    return 1;
}

int LuaBinding::navSave(lua_State* luaVM)
{ 
    if (lua_type(luaVM, 1) != LUA_TSTRING) {
        lua_pushboolean(luaVM, false);
        return 1;
    }

    auto& navigation = Navigation::GetInstance();
    
    const char* path = lua_tostring(luaVM, 1);
    const bool result = navigation.Save(path);

    lua_pushboolean(luaVM, result);
    return 1;
}

int LuaBinding::navFindPath(lua_State* luaVM)
{
    if (lua_gettop(luaVM) != 6) {
        return luaL_error(luaVM, "expecting exactly 6 arguments");
    }

    auto& navigation = Navigation::GetInstance();
    auto* navmesh = navigation.GetNavMesh();
    if (!navmesh) {
        lua_pushboolean(luaVM, false);
        return 1;
    }

    Vector3F pointStart;
	Vector3F pointEnd;
	Vector3F extents(2.0f, 2.0f, 2.0f);

	pointStart.x_ = static_cast<float>(lua_tonumber(luaVM, -6));
	pointStart.z_ = static_cast<float>(lua_tonumber(luaVM, -5));
	pointStart.y_ = static_cast<float>(lua_tonumber(luaVM, -4));
	pointEnd.x_ = static_cast<float>(lua_tonumber(luaVM, -3));
	pointEnd.z_ = static_cast<float>(lua_tonumber(luaVM, -2));
	pointEnd.y_ = static_cast<float>(lua_tonumber(luaVM, -1));

    std::vector<Vector3F> path;
    navmesh->FindPath(path, pointStart, pointEnd, extents);

    if (path.empty()) {
        lua_pushboolean(luaVM, false);
        return 1;
    }

    lua_newtable(luaVM);

    for (size_t i = 0; i < path.size(); ++i) {
        const Vector3F& point = path[i];

		lua_pushnumber(luaVM, i + 1);   /* Push the table index */

		lua_newtable(luaVM);
		lua_pushnumber(luaVM, 1);
		lua_pushnumber(luaVM, point.x_);
		lua_rawset(luaVM, -3);
		lua_pushnumber(luaVM, 2);
		lua_pushnumber(luaVM, point.z_);
		lua_rawset(luaVM, -3);
		lua_pushnumber(luaVM, 3);
		lua_pushnumber(luaVM, point.y_);
		lua_rawset(luaVM, -3);

		//lua_pushnumber(lua_vm, i * 2); /* Push the cell value */
		lua_rawset(luaVM, -3);      /* Stores the pair in the table */
    }
      
    return 1;
}

int LuaBinding::navNearestPoint(lua_State* luaVM)
{
    if (lua_gettop(luaVM) != 3) {
        return luaL_error(luaVM, "expecting exactly 3 arguments");
    }

    auto& navigation = Navigation::GetInstance();
    auto* navmesh = navigation.GetNavMesh();
    if (!navmesh) {
        lua_pushboolean(luaVM, false);
        return 1;
    }

    Vector3F point;
	Vector3F extents(2.0f, 2.0f, 2.0f);

	point.x_ = static_cast<float>(lua_tonumber(luaVM, -3));
	point.z_ = static_cast<float>(lua_tonumber(luaVM, -2));
	point.y_ = static_cast<float>(lua_tonumber(luaVM, -1));

    dtPolyRef nearestRef{};

    Vector3F result = navmesh->FindNearestPoint(point, extents, nullptr, &nearestRef);        
      
    if (nearestRef) {
        lua_pushnumber(luaVM, result.x_);
		lua_pushnumber(luaVM, result.z_);
		lua_pushnumber(luaVM, result.y_);
		return 3;
    }

    lua_pushboolean(luaVM, false);
    return 1;
}

int LuaBinding::navDump(lua_State* luaVM)
{
    if (lua_type(luaVM, 1) != LUA_TSTRING) {
        lua_pushboolean(luaVM, false);
        return 1;
    }

    auto& navigation = Navigation::GetInstance();
    
    const char* path = lua_tostring(luaVM, 1);
    const bool result = navigation.Dump(path);

    lua_pushboolean(luaVM, result);
    return 1;
}

int LuaBinding::navBuild(lua_State* luaVM)
{
    auto& navigation = Navigation::GetInstance(); 
    auto* navmesh = navigation.GetNavMesh();
    if (!navmesh) {
        lua_pushboolean(luaVM, false);
        return 1;
    }

    const bool result = navmesh->Build();
    lua_pushboolean(luaVM, result);
    return 1; 
}

int LuaBinding::navCollisionMesh(lua_State* luaVM)
{
    if (lua_gettop(luaVM) != 7) {
        return luaL_error(luaVM, "expecting exactly 7 arguments");
    }

    Vector3F min;
	Vector3F max;
	min.x_ = static_cast<float>(lua_tonumber(luaVM, -7));
	min.z_ = static_cast<float>(lua_tonumber(luaVM, -6));
	min.y_ = static_cast<float>(lua_tonumber(luaVM, -5));
	max.x_ = static_cast<float>(lua_tonumber(luaVM, -4));
	max.z_ = static_cast<float>(lua_tonumber(luaVM, -3));
	max.y_ = static_cast<float>(lua_tonumber(luaVM, -2));
    const float bias = static_cast<float>(lua_tonumber(luaVM, -1));

    BoundingBox bounds;
    bounds.Merge(min);
    bounds.Merge(max);

    auto& navigation = Navigation::GetInstance();
    World* world = navigation.GetWorld();
    Scene* scene = world->GetScene();

    std::vector<const SceneNode*> result;
	scene->Query(&bounds.min_.x_, result);

    std::vector<Vector3F> vertices;
    std::vector<std::int32_t> indices;

    for (const auto& node : result) {
        auto* collision = world->GetModelCollision(node->GetModel());
		if (!collision || collision->Empty()) {
			continue;
		}

		collision->Unpack(vertices, indices, node->GetTransform(), static_cast<std::int32_t>(vertices.size()));
    }

    DebugMesh mesh(std::move(vertices), std::move(indices));
    const auto primitives = mesh.GetTriangleList();

    lua_newtable(luaVM);

    for (size_t i = 0; i < primitives.size(); ++i) {
        const Vector3F& point = primitives[i];

		lua_pushnumber(luaVM, i + 1);   /* Push the table index */

		lua_newtable(luaVM);
		lua_pushnumber(luaVM, 1);
		lua_pushnumber(luaVM, point.x_);
		lua_rawset(luaVM, -3);
		lua_pushnumber(luaVM, 2);
		lua_pushnumber(luaVM, point.z_);
		lua_rawset(luaVM, -3);
		lua_pushnumber(luaVM, 3);
		lua_pushnumber(luaVM, point.y_ + bias);
		lua_rawset(luaVM, -3);

		lua_rawset(luaVM, -3);      /* Stores the pair in the table */
    }
      
    return 1;
}

int LuaBinding::navNavigationMesh(lua_State* luaVM)
{
    if (lua_gettop(luaVM) != 7) {
        return luaL_error(luaVM, "expecting exactly 7 arguments");
    }
 
    auto* navmesh = Navigation::GetInstance().GetNavMesh();
    if (!navmesh) {
        lua_pushboolean(luaVM, false);
        return 1;
    }

    Vector3F min;
	Vector3F max;
	min.x_ = static_cast<float>(lua_tonumber(luaVM, -7));
	min.z_ = static_cast<float>(lua_tonumber(luaVM, -6));
	min.y_ = static_cast<float>(lua_tonumber(luaVM, -5));
	max.x_ = static_cast<float>(lua_tonumber(luaVM, -4));
	max.z_ = static_cast<float>(lua_tonumber(luaVM, -3));
	max.y_ = static_cast<float>(lua_tonumber(luaVM, -2));
    const float bias = static_cast<float>(lua_tonumber(luaVM, -1));

    BoundingBox bounds;
    bounds.Merge(min);
    bounds.Merge(max);    

    DebugMesh mesh;
    navmesh->Dump(mesh, true, &bounds);

    const auto primitives = mesh.GetTriangleList();

    lua_newtable(luaVM);

    for (size_t i = 0; i < primitives.size(); ++i) {
        const Vector3F& point = primitives[i];

		lua_pushnumber(luaVM, i + 1);   /* Push the table index */

		lua_newtable(luaVM);
		lua_pushnumber(luaVM, 1);
		lua_pushnumber(luaVM, point.x_);
		lua_rawset(luaVM, -3);
		lua_pushnumber(luaVM, 2);
		lua_pushnumber(luaVM, point.z_);
		lua_rawset(luaVM, -3);
		lua_pushnumber(luaVM, 3);
		lua_pushnumber(luaVM, point.y_ + bias);
		lua_rawset(luaVM, -3);

		lua_rawset(luaVM, -3);      /* Stores the pair in the table */
    }
      
    return 1;
}

int LuaBinding::navScanWorld(lua_State* luaVM)
{
    if (lua_gettop(luaVM) != 6) {
        return luaL_error(luaVM, "expecting exactly 6 arguments");
    }

    Vector3F min;
	Vector3F max;
	min.x_ = static_cast<float>(lua_tonumber(luaVM, -6));
	min.z_ = static_cast<float>(lua_tonumber(luaVM, -5));
	min.y_ = static_cast<float>(lua_tonumber(luaVM, -4));
	max.x_ = static_cast<float>(lua_tonumber(luaVM, -3));
	max.z_ = static_cast<float>(lua_tonumber(luaVM, -2));
	max.y_ = static_cast<float>(lua_tonumber(luaVM, -1));

    BoundingBox bounds;
    bounds.Merge(min);
    bounds.Merge(max);

    auto& navigation = Navigation::GetInstance();
    World* world = navigation.GetWorld();
    Scene* scene = world->GetScene();

    std::vector<const SceneNode*> result;
	scene->Query(&bounds.min_.x_, result);

    lua_newtable(luaVM);

    for (size_t i = 0; i < result.size(); ++i) {
        const SceneNode* node = result[i];

		lua_pushnumber(luaVM, i + 1);   /* Push the table index */

		lua_newtable(luaVM);
		lua_pushnumber(luaVM, 1);
		lua_pushinteger(luaVM, node->GetModel());
		lua_rawset(luaVM, -3);	

		lua_rawset(luaVM, -3);      /* Stores the pair in the table */
    }
      
    return 1;
}

}

#pragma warning( pop )