workspace "mta-navigation"
    configurations { "Debug", "Release" }
    platforms { "x86", "x64" }

    location "Build"
    startproject "builder"

    language "C++"
    staticruntime "off"

    defines {  
        "_CRT_SECURE_NO_WARNINGS",
        "NAVIGATION_EXPORT"
    }

    newoption {
        trigger = "navapi",
        value = "API",
        description = "Choose a particular API for export",
        allowed = {
           { "lua",    "MTA Lua" },
           { "native",  "Native C" },
        },
        default = "lua"
    }    
    
    filter "platforms:x86"
		architecture "x86"
	filter "platforms:x64"
		architecture "x86_64"

    filter "system:linux"
        cppdialect "C++2a"
        links "pthread"
    filter "system:windows"
        cppdialect "C++20"
        largeaddressaware "on"
        defaultplatform "x86"

    filter "system:windows"
        defines "WIN32"     

    filter "configurations:Release"
		optimize "Speed"

    filter {"system:windows", "configurations:Debug"}
        runtime "Debug"
    filter {"system:windows", "configurations:Release"}        
        runtime "Release"       

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

                -- Vendor
                "vendor"
            }

            links {
                "recastnavigation",
                "pugixml",
                "LZ4"
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
              
                -- Vendor
                "vendor"
            }

            links {
                "LZ4",
                "recastnavigation",
                "pugixml"        
            }

            filter { "options:navapi=native" }
                defines "EXPORT_NATIVE_API"

            filter { "options:navapi=lua" }
                links "module-sdk"
                includedirs "vendor/lua"
                defines "EXPORT_LUA_API"

            filter {"system:windows", "platforms:x86", "options:navapi=lua" }
                links "vendor/lua/lib/lua5.1.lib"
            filter {"system:windows", "platforms:x64", "options:navapi=lua" }
                links "vendor/lua/lib/lua5.1_64.lib"
       
            filter "configurations:Debug"
                defines { "DEBUG" }
                symbols "On"

            filter "configurations:Release"
                defines { "NDEBUG" }
                optimize "On"

    group "Vendor"
        include "vendor/recastnavigation"
        include "vendor/LZ4"
        include "vendor/pugixml"

        if _OPTIONS["navapi"] == "lua" then
            include "vendor/module-sdk"
        end

