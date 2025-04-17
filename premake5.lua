-- Globals
-----------------------------
function GetAllModuleFiles(directory)
    print("Checking Directory " .. directory)
    local ifc_files = os.matchfiles(directory .. "/*")
    local file_count = 0
    local mod_refs = {}
    
    for _, file in ipairs(ifc_files) do 
        table.insert(mod_refs, "/reference " .. file)
        file_count = file_count + 1
    end
    
    print("File Count: " .. file_count)

    return mod_refs
end
-----------------------------

-- Workspace Declarations
-----------------------------
workspace "TerrainGenerator"
    architecture "x64"
    startproject "Sandbox"

    configurations { "Debug", "Release", "Dist"}

    outputdir = "%{cfg.buildcfg}_%{cfg.system}_%{cfg.architecture}"

    VulkanSDK = os.getenv("VULKAN_SDK")

    IncludeDirs = {}
    IncludeDirs["AurionCore"] = "%{wks.location}/third_party/aurion-core/include/aurion-core"
    IncludeDirs["GLFW"] = "%{wks.location}/third_party/GLFW/include"
    IncludeDirs["ImGui"] = "%{wks.location}/third_party/imgui"
    IncludeDirs["Vulkan"] = "%{VulkanSDK}/Include"

    LibDirs = {}
    LibDirs["AurionCore"] = "%{wks.location}/third_party/aurion-core/lib"
    LibDirs["GLFW"] = "%{wks.location}/third_party/GLFW/lib"
    LibDirs["Vulkan"] = "%{VulkanSDK}/Lib"

    -- Compile .ixx files as Module Interface Units
    filter { "files:**.ixx" }
        compileas "Module"
    filter {} -- Reset filter to prevent overlap

    -- Core Project Declaration
    project "TerrainGenerator"
        kind "ConsoleApp"
        language "C++"
        cppdialect "C++20"

        -- Build Directories
        targetdir ("%{wks.location}/build/bin/" .. outputdir .. "/%{prj.name}")
        objdir ("%{wks.location}/build/bin-int/" .. outputdir .. "/%{prj.name}")

        -- Ensure external dependencies aren't build with the project
        filter "files:third_party/aurion-core/**"
            buildaction "None"
        filter{}

        -- C++ Module Support
        allmodulespublic "On"
        scanformoduledependencies "true"

        -- File Locations
        files { 
            "%{IncludeDirs.AurionCore}/**.ixx",
            "%{IncludeDirs.AurionCore}/**.h",
            "%{IncludeDirs.ImGui}/*.h",
            "%{IncludeDirs.ImGui}/backends/imgui_impl_glfw.h",
            "%{IncludeDirs.ImGui}/backends/imgui_impl_vulkan.h",
            "%{IncludeDirs.ImGui}/*.cpp",
            "%{IncludeDirs.ImGui}/backends/imgui_impl_glfw.cpp",
            "%{IncludeDirs.ImGui}/backends/imgui_impl_vulkan.cpp",
            "%{IncludeDirs.ImGui}/misc/debuggers/imgui.natvis",
            "%{IncludeDirs.ImGui}/misc/debuggers/imgui.natstepfilter",
            "%{IncludeDirs.ImGui}/cpp/imgui_stdlib.*",
            "core/**.h", 
            "core/**.ixx", 
            "core/**.cpp"
        }

        -- Include Directories
        includedirs { 
            "core", 
            IncludeDirs["AurionCore"],
            IncludeDirs["GLFW"],
            IncludeDirs["ImGui"],
            IncludeDirs["Vulkan"],
        }

        -- Library Directories 
        libdirs { 
            LibDirs["AurionCore"],
            LibDirs["GLFW"],
            LibDirs["Vulkan"],
        }

        links {
            "glfw3dll.lib",
            "vulkan-1.lib"
         }

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

            postbuildcommands {
                "{COPY} third_party/aurion-core/lib/debug/AurionCore.dll %{wks.location}/build/bin/" .. outputdir .. "/%{prj.name}/",
                "{COPY} third_party/GLFW/lib/glfw3.dll %{wks.location}/build/bin/" .. outputdir .. "/%{prj.name}/"
            }

            links {
                "debug/AurionCore.lib",
                "shaderc_sharedd.lib",
                "spirv-cross-c-sharedd.lib",
                "spirv-cross-cored.lib",
                "spirv-cross-glsld.lib",
            }

            defines { "AURION_CORE_DEBUG" }

        filter "configurations:Release"
            staticruntime "Off"
            runtime "Release"
            optimize "On"

            buildoptions{ "/MD", GetAllModuleFiles("third_party/aurion-core/lib/modules")}
            postbuildcommands {
                "{COPY} third_party/aurion-core/lib/AurionCore.dll %{wks.location}/build/bin/" .. outputdir .. "/%{prj.name}/",
                "{COPY} third_party/GLFW/lib/glfw3.dll %{wks.location}/build/bin/" .. outputdir .. "/%{prj.name}/"
            }

            links { 
                "AurionCore.lib",
                "shaderc_shared.lib",
                "spirv-cross-c-shared.lib",
                "spirv-cross-core.lib",
                "spirv-cross-glsl.lib"
            }

        filter "configurations:Dist"
            staticruntime "Off"
            runtime "Release"
            optimize "On"

            buildoptions{ "/MD", GetAllModuleFiles("third_party/aurion-core/lib/modules")}
            postbuildcommands {
                "{COPY} third_party/aurion-core/lib/AurionCore.dll %{wks.location}/build/bin/" .. outputdir .. "/%{prj.name}/",
                "{COPY} third_party/GLFW/lib/glfw3.dll %{wks.location}/build/bin/" .. outputdir .. "/%{prj.name}/"
            }

            links { 
                "AurionCore.lib",
                "shaderc_shared.lib",
                "spirv-cross-c-shared.lib",
                "spirv-cross-core.lib",
                "spirv-cross-glsl.lib"
            }