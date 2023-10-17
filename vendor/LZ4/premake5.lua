project "LZ4"
	language "C++"
	kind "StaticLib"
    targetname "LZ4" 
	pic "On"

    files {
		"premake5.lua",
		"lz4.c",
		"lz4hc.c",
    }