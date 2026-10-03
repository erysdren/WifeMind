cmake_minimum_required(VERSION 3.19)
project(WifeMind
	DESCRIPTION "Advanced and portable idTech engine"
	HOMEPAGE_URL "https://github.com/erysdren/WifeMind"
	LANGUAGES C CXX
)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# global

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

cmake_policy(SET CMP0072 NEW) # prefer GLVND
cmake_policy(SET CMP0135 NEW) # set FetchContent URL timestamps based on time of extraction
cmake_policy(SET CMP0115 NEW) # prefer explicitly set source file extension

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
cmake_dependent_option(FTE_PLUGIN_EZHUD "Build EzHud plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_HL2 "Build HL2 formats plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_IRC "Build IRC plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_JOLT "Build Jolt physics plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_MODELS "Build Model formats plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_MPQ "Build MPQ format plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_NAMEMAKER "Build NarbDrop formats plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_NARBDROP "Build NarbDrop formats plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_ODE "Build ODE physics plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_OPENSSL "Build OpenSSL plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_OPENXR "Build OpenXR plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_QUAKE3 "Build Quake 3 plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_QI "Build Quaddicted Database plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_ROTT "Build ROTT formats plugin" ON FTE_PLUGINS OFF)
cmake_dependent_option(FTE_PLUGIN_SATURN "Build Sega Saturn Quake formats plugin" OFF FTE_PLUGINS OFF) # NOTE: unfinished
cmake_dependent_option(FTE_PLUGIN_TERRAINGEN "Build sample Terrain Generator plugin" ON FTE_PLUGINS OFF)
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
option(FTE_ENGINE_USE_DXVK "Use DXVK instead of Direct3D" OFF)
option(FTE_ENGINE_USE_PNG "Use libpng for PNG textures" ON)

set(FTE_ENGINE_SDL_VERSION_MAJOR 3 CACHE STRING "Major SDL version to use for engine")
set_property(CACHE FTE_ENGINE_SDL_VERSION_MAJOR PROPERTY STRINGS 1 2 3)

set(FTE_ENGINE_RENDERER gl CACHE STRING "Renderer to use for engine")
set_property(CACHE FTE_ENGINE_RENDERER PROPERTY STRINGS gl vk d3d8 d3d9 d3d11 sw)

if(FTE_ENGINE_USE_DXVK AND NOT FTE_ENGINE_RENDERER STREQUAL d3d8 AND NOT FTE_ENGINE_RENDERER STREQUAL d3d9 AND NOT FTE_ENGINE_RENDERER STREQUAL d3d11)
	message(WARNING "Forcing DXVK off, since a Direct3D renderer was not specified")
	set(FTE_ENGINE_USE_DXVK OFF)
endif()

set(FTE_ENGINE_CLIENT_NAME "fteqw" CACHE STRING "Name of client binary")
set(FTE_ENGINE_SERVER_NAME "fteqw-sv" CACHE STRING "Name of server binary")

# common options

set(FTE_COMMON_LINK_OPTIONS "")
set(FTE_COMMON_OPTIONS "")
set(FTE_COMMON_DEFINITIONS "")

list(APPEND FTE_COMMON_OPTIONS
	$<$<AND:$<C_COMPILER_ID:GNU,Clang>,$<CONFIG:Release>>:-O3>
	$<$<AND:$<C_COMPILER_ID:GNU,Clang>,$<CONFIG:MinSizeRel>>:-Os>
	$<$<AND:$<C_COMPILER_ID:GNU,Clang>,$<CONFIG:Debug>>:-O0>
)

list(APPEND FTE_COMMON_DEFINITIONS
	$<$<CONFIG:Debug>:_DEBUG>
)

list(APPEND FTE_COMMON_LINK_OPTIONS
	$<$<AND:$<C_COMPILER_ID:GNU,Clang>,$<CONFIG:Release,MinSizeRel>>:-s>
	$<$<BOOL:${MINGW}>:-static>
	$<$<BOOL:${MINGW}>:-static-libgcc>
	$<$<BOOL:${MINGW}>:-static-libstdc++>
	$<$<BOOL:${MINGW}>:-mcrtdll=msvcrt20> # was msvcrt-os
)

list(APPEND FTE_COMMON_DEFINITIONS
	$<$<BOOL:${NXDK}>:_XBOX>
	$<$<BOOL:${KOS}>:__KOS__>
	$<$<BOOL:${KOS}>:__DREAMCAST__>
	$<$<BOOL:${MINGW}>:__MSVCRT_VERSION__=0x200> # was 0x700
	$<$<BOOL:${MINGW}>:_WIN32_WINNT=0x0500> # was 0x0501
	# kindve a hack
	ARCH_CPU_POSTFIX=\"\"
	ARCH_DL_POSTFIX=\"${FTE_SHARED_LIBRARY_SUFFIX}\"
)

# external dependencies

include(fte_dependencies)

# vars

include(fte_revision)
include(fte_system)
include(fte_directories)
include(fte_config)

# common options helper function
# NOTE: this intentionally comes after all the vars were set by the above includes

function(fte_add_common target)
	target_compile_options(${target}
		PRIVATE
			${FTE_COMMON_OPTIONS}
	)
	target_link_options(${target}
		PRIVATE
			${FTE_COMMON_LINK_OPTIONS}
	)
	target_compile_definitions(${target}
		PRIVATE
			${FTE_COMMON_DEFINITIONS}
	)
endfunction()

# components

include(fte_tools)
include(fte_plugins)
include(fte_engine)
