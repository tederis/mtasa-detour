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

	includedirs {
		"..",
		"../lua"
	}

	filter {"system:windows" }
		disablewarnings { "4267" }

	filter {"system:windows", "platforms:x86" }
		links "../lua/lib/lua5.1.lib"
	filter {"system:windows", "platforms:x64" }
		links "../lua/lib/lua5.1_64.lib"	