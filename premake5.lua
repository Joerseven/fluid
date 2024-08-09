workspace "Fluid"
  configurations { "Debug", "Release" }
  platforms { "Win64", "Mac"}

  filter "system:Win64"
    system "windows"
    architecture "x86_64"

  filter "system:Mac"
    system "macosx"

project "Fluid"
  kind "ConsoleApp"
  language "C++"
  targetdir "build/%{cfg.buildcfg}"
  libdirs { "external/raylib/lib" }
  links { "raylib" }
  includedirs { "external/raylib/include", "src/" }
  buildoptions { "-std=c++17" }

  files { "**.h", "**.cpp" }

  filter "configurations:Debug"
    defines { "DEBUG" }
    symbols "On"

  filter "configurations:Release"
    defines { "NDEBUG" }
    optimize "On"

  filter "system:windows"
    linkoptions { "/NODEFAULTLIB:MSVCRT" }
    links { "winmm" }

  filter "system:macosx"
    linkoptions { "-rpath @executable_path/../../external/raylib/lib"}



