project "pugixml"
	language "C++"
	kind "StaticLib"
    targetname "pugixml" 
	pic "On"

    files {
		"premake5.lua",
		"src/pugixml.cpp"
    }