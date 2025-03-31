-- Workspace Declarations
workspace "TerrainGenerator"
    architecture "x64"
    startproject "Sandbox"

    configurations { "Debug", "Release", "Dist"}

    outputdir = "%{cfg.buildcfg}_%{cfg.system}_%{cfg.architecture}"

    -- Module Interface Units
    filter { "files:**.ixx" }
        compileas "Module"
    filter {} -- Reset filter to prevent overlap

function GetAllModuleFiles(directory)
    print("Checking Directory " .. directory)
    local ifc_files = os.matchfiles(directory .. "/*")
    local mod_refs = {}

    for _, file in ipairs(ifc_files) do 
        table.insert(mod_refs, "/reference " .. file)
    end
    
    return mod_refs
end

-- Function to generate core solution project
function GenerateCoreSolution()
    print("Generating Solution: Terrain Generator")

    -- Core Project Declaration
    project "TerrainGenerator"
        kind "ConsoleApp"
        language "C++"
        cppdialect "C++20"

        -- Build Directories
        targetdir ("%{wks.location}/build/bin/" .. outputdir .. "/%{prj.name}")
        objdir ("%{wks.location}/build/bin-int/" .. outputdir .. "/%{prj.name}")

        -- Ensure external dependencies aren't build with the project
        filter "files:third_party/**"
            buildaction "None"
        filter{}

        -- C++ Module Support
        allmodulespublic "On"
        scanformoduledependencies "true"

        -- File Locations
        files { "**.h", "**.ixx", "**.cpp" }

        -- Include Directories
        includedirs { "core", "third_party/*/include" }

        -- Library Directories 
        libdirs { "third_party/*/lib" }

        links { "glfw3dll.lib" }

        -- Platform (OS) Filters
        filter "system:windows"
		    systemversion "latest"

            defines { "AURION_PLATFORM_WINDOWS" }

        -- Build Configuration Filters
        filter "configurations:Debug"
            staticruntime "Off"
            runtime "Debug"
            symbols "On"

            buildoptions{ "/MDd", GetAllModuleFiles("third_party/aurion-core/lib/debug/modules")}

            links { "debug/AurionCored.lib" }

            defines { "AURION_CORE_DEBUG" }

        filter "configurations:Release"
            staticruntime "Off"
            runtime "Release"
            optimize "On"

           buildoptions{ "/MD", GetAllModuleFiles("third_party/aurion-core/lib/modules")}

            links { "AurionCore.lib" }

        filter "configurations:Dist"
            staticruntime "Off"
            runtime "Release"
            optimize "On"

            buildoptions{ "/MD", GetAllModuleFiles("third_party/aurion-core/lib/modules")}

            links { "AurionCore.lib" }

        
end

-- Generate Core solution
GenerateCoreSolution()