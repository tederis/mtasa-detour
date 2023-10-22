project "Lua"
	language "C++"
	kind "StaticLib"
	targetname "vlua"
	pic "On"

	vpaths { 
		["Headers"] = "**.h",
		["Sources"] = "**.c",
		["Sources"] = "**.cpp",
		["*"] = "premake5.lua"
	}
	
	files {
		"premake5.lua",
		"src/**.c",
		"src/**.cpp",
		"src/**.h"
	}

	defines "VLUA_EXPORT"

	filter "system:windows"
		defines 'VLUA_LIB_NAME="lua5.1"'
	filter { "not system:windows" }
		defines "VLUA_LIB_NAME=\"deathmatch\""

	filter {"platforms:x64"}
		defines "VLUA_PLATFORM=\"x64/\""
	filter {"platforms:x86", "system:windows" }
		defines "VLUA_PLATFORM="
	filter {"platforms:x86", "system:linux" }
		defines "VLUA_PLATFORM=\"mods/deathmatch/\""