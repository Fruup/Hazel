outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] = "Hazel/vendor/GLFW/include"
IncludeDir["Glad"] = "Hazel/vendor/Glad/include"
IncludeDir["ImGui"] = "Hazel/vendor/imgui"
IncludeDir["glm"] = "Hazel/vendor/glm"
IncludeDir["stb_image"] = "Hazel/vendor/stb_image"
IncludeDir["enet"] = "Hazel/vendor/enet/include"

group "Hazel/Dependencies"
	include "Hazel/vendor/GLFW"
	include "Hazel/vendor/Glad"
	include "Hazel/vendor/imgui"
	include "Hazel/vendor/enet"

group "Hazel"
	project "Hazel"
		location "Hazel"
		kind "StaticLib"
		language "C++"
		cppdialect "C++17"
		staticruntime "on"

		targetdir ("bin/" .. outputdir .. "/%{prj.name}")
		objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

		pchheader "hzpch.h"
		pchsource "Hazel/src/hzpch.cpp"

		files {
			"%{prj.name}/src/**.h",
			"%{prj.name}/src/**.cpp",
			"%{prj.name}/vendor/stb_image/**.h",
			"%{prj.name}/vendor/stb_image/**.cpp",
			"%{prj.name}/vendor/glm/glm/**.hpp",
			"%{prj.name}/vendor/glm/glm/**.inl",
		}

		defines {
			"_CRT_SECURE_NO_WARNINGS",
			"_SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING"
		}

		includedirs {
			"%{prj.name}/src",
			"%{prj.name}/vendor/spdlog/include",
			"%{IncludeDir.GLFW}",
			"%{IncludeDir.Glad}",
			"%{IncludeDir.ImGui}",
			"%{IncludeDir.glm}",
			"%{IncludeDir.stb_image}",
			"%{IncludeDir.enet}"
		}

		links  { 
			"GLFW",
			"Glad",
			"ImGui",
			"enet_static",
			"opengl32.lib"
		}

		filter "system:windows"
			systemversion "latest"

			defines {
				"HZ_BUILD_DLL",
				"GLFW_INCLUDE_NONE"
			}

			links {
				"ws2_32.lib",
				"Winmm.lib"
			}

		filter "configurations:Debug"
			defines { "HZ_DEBUG", "ENET_DEBUG" }
			runtime "Debug"
			symbols "on"

		filter "configurations:Release"
			defines "HZ_RELEASE"
			runtime "Release"
			optimize "on"

		filter "configurations:Dist"
			defines "HZ_DIST"
			runtime "Release"
			optimize "on"
