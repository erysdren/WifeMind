cmake_minimum_required(VERSION 3.19)
project(fteqw-ng
	DESCRIPTION "Advanced and portable idTech engine"
	HOMEPAGE_URL "https://github.com/erysdren/fteqw"
	LANGUAGES C CXX
)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# global

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

cmake_policy(SET CMP0072 NEW)
cmake_policy(SET CMP0135 NEW)
cmake_policy(SET CMP0115 NEW)

if(MINGW)
	add_link_options(
		-static
		-static-libgcc
		-static-libstdc++
		-mcrtdll=msvcrt-os
	)
endif()

# options

include(CMakeDependentOption)

option(FTE_TOOLS "Build tools" ON)
cmake_dependent_option(FTE_TOOL_QTV "Build fteqtv" OFF FTE_TOOLS OFF) # NOTE: seems unfinished
cmake_dependent_option(FTE_TOOL_IMG "Build imgtool" OFF FTE_TOOLS OFF) # NOTE: currently broken
cmake_dependent_option(FTE_TOOL_IQM "Build iqmtool" ON FTE_TOOLS OFF)
cmake_dependent_option(FTE_TOOL_ASM "Build q3asm2" ON FTE_TOOLS OFF)
cmake_dependent_option(FTE_TOOL_QCC "Build fteqcc" ON FTE_TOOLS OFF)
cmake_dependent_option(FTE_TOOL_QCCGUI "Build fteqccgui" ON "FTE_TOOLS;WIN32" OFF)
cmake_dependent_option(FTE_TOOL_MASTER "Build ftemaster" ON FTE_TOOLS OFF)
cmake_dependent_option(FTE_TOOL_HTTP "Build httpserver" ON FTE_TOOLS OFF)
cmake_dependent_option(FTE_TOOL_HEIGHTMAPCONVERTER "Build heightmapconverter" ON FTE_TOOLS OFF)

option(FTE_PLUGINS "Build plugins" ON)
cmake_dependent_option(FTE_PLUGIN_BOX3D "Build Box3D physics plugin" OFF FTE_PLUGINS OFF) # NOTE: cant build yet
cmake_dependent_option(FTE_PLUGIN_BULLET "Build Bullet physics plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_COD "Build CoD formats plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_HL2 "Build HL2 formats plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_IRC "Build IRC plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_JOLT "Build Jolt physics plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_NARBDROP "Build NarbDrop formats plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_ODE "Build ODE physics plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_OPENXR "Build OpenXR plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_QUAKE3 "Build Quake 3 plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_ROTT "Build ROTT formats plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_SATURN "Build Sega Saturn Quake formats plugin" OFF FTE_PLUGINS OFF) # NOTE: unfinished
cmake_dependent_option(FTE_PLUGIN_WINAMP "Build Winamp plugin" ON "FTE_PLUGINS;WIN32" OFF)
cmake_dependent_option(FTE_PLUGIN_XMPP "Build XMPP plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_XSV "Build X11 server plugin" ON "FTE_PLUGINS;UNIX" OFF)

option(FTE_ENGINE "Build engine" ON)
cmake_dependent_option(FTE_ENGINE_CLIENT "Build server+client engine" ON FTE_ENGINE OFF)
cmake_dependent_option(FTE_ENGINE_SERVER "Build server-only engine" ON FTE_ENGINE OFF)

option(FTE_VENDOR_DEPENDENCIES "Vendor all thirdparty dependencies" ON)
option(FTE_ENGINE_USE_FREETYPE "Use freetype for TTF fonts" ON)
option(FTE_ENGINE_USE_OGGVORBIS "Use libogg/libvorbis for sounds" ON)
option(FTE_ENGINE_USE_SDL "Use SDL for platform abstraction" ON)

set(FTE_ENGINE_SDL_VERSION_MAJOR 3 CACHE STRING "Major SDL version to use for engine")
set_property(CACHE FTE_ENGINE_SDL_VERSION_MAJOR PROPERTY STRINGS 1 2 3)

set(FTE_ENGINE_RENDERER gl CACHE STRING "Renderer to use for engine")
set_property(CACHE FTE_ENGINE_RENDERER PROPERTY STRINGS gl vk d3d d3d8 sw)

set(FTE_ENGINE_CLIENT_NAME "fteqw" CACHE STRING "Name of client binary")
set(FTE_ENGINE_SERVER_NAME "fteqw-sv" CACHE STRING "Name of server binary")

# compile options

set(FTE_COMMON_OPTIONS "")
set(FTE_COMMON_DEFINITIONS "")

# external dependencies

include(fte_dependencies)

# vars

include(fte_revision)
include(fte_system)
include(fte_directories)
include(fte_config)

# components

include(fte_tools)
include(fte_plugins)
include(fte_engine)
