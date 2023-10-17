workspace "mta-navigation"
    configurations { "Debug", "Release" }   

    language "C++"    

    staticruntime "off"    

    filter "system:linux"
        cppdialect "C++2a"
        links "pthread"
    filter "system:windows"
        cppdialect "C++20"
        largeaddressaware "on"

    filter {}

    location "Build"
    startproject "builder"

    filter {"system:windows", "configurations:Debug"}
        runtime "Debug"

    filter {"system:windows", "configurations:Release"}        
        runtime "Release"

    group "Vendor"
        include "vendor/recastnavigation"
        include "vendor/LZ4"
        include "vendor/pugixml"
        include "vendor/Lua"

    group "Navigation"
        project "builder"
            kind "ConsoleApp"            
            targetdir "bin/%{cfg.buildcfg}"

            files { 
                "source/builder/**",
                "source/utils/**",
                "source/scene/**",
                "source/navigation/**",
                "source/game/**",
            }

            includedirs {
                -- recastnavigation
                "vendor/recastnavigation/Detour/Include",
                "vendor/recastnavigation/DetourTileCache/Include",
                "vendor/recastnavigation/Recast/Include",

                -- spdlog
                "vendor/spdlog/include",

                -- glm
                "vendor/glm",

                -- pugixml
                "vendor/pugixml/src",

                "vendor"
            }

            links {
                "recastnavigation",
                "pugixml",
                "LZ4"
            }

            filter "system:windows"
                defines "WIN32"

            filter {}
                defines {  
                    "_CRT_SECURE_NO_WARNINGS"
                }

            filter "configurations:Debug"
                defines { "DEBUG" }
                symbols "On"

            filter "configurations:Release"
                defines { "NDEBUG" }
                optimize "On"

        project "navigation"
            kind "SharedLib"
            targetdir "bin/%{cfg.buildcfg}"

            files { 
                "source/module/**",
                "source/utils/**",
                "source/scene/**",
                "source/navigation/**",
                "source/game/**",
            }

            includedirs {
                -- recastnavigation
                "vendor/recastnavigation/Detour/Include",
                "vendor/recastnavigation/DetourTileCache/Include",
                "vendor/recastnavigation/Recast/Include",

                -- spdlog
                "vendor/spdlog/include",

                -- glm
                "vendor/glm",

                -- pugixml
                "vendor/pugixml/src",

                -- Lua
                "vendor/lua/src",

                "vendor"
            }

            links {
                "LZ4",
                "recastnavigation",
                "pugixml",
                "Lua"
            }

            filter "system:windows"
                defines "WIN32"

            filter {}
                defines {  
                    "_CRT_SECURE_NO_WARNINGS"
                }

            filter "configurations:Debug"
                defines { "DEBUG" }
                symbols "On"

            filter "configurations:Release"
                defines { "NDEBUG" }
                optimize "On"

