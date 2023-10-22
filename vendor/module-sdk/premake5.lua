project "module-sdk"
	language "C++"
	kind "StaticLib"
    targetname "module-sdk" 
	pic "On"

    files {
		"premake5.lua",
		"*.h",
		"*.cpp",
		"extra/*.cpp",
		"extra/*.h"
    }

	links "Lua"
	includedirs "../lua/src"

	filter {"system:windows" }
		disablewarnings { "4267" }