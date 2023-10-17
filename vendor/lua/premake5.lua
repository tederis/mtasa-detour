project "Lua"
	language "C++"
	kind "StaticLib"
    targetname "lua5.1" 
	pic "On"

    files {
		"premake5.lua",
		"src/**.c",
		"src/**.h",
	}

	defines {
		"_CRT_SECURE_NO_WARNINGS"
	}
