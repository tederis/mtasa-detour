project "recastnavigation"
    language "C++"
	kind "StaticLib"
    targetname "recastnavigation"
    pic "On"
    
    includedirs {
        "Detour/Include",
        "DetourTileCache/Include",
        "Recast/Include",
    }

    files {
		"premake5.lua",
		"Detour/**.h",
		"Detour/**.cpp",
		"DetourTileCache/**.h",
		"DetourTileCache/**.cpp",
		"Recast/**.h",
		"Recast/**.cpp",
    }