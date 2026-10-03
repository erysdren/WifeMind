
include(FetchContent)

find_package(Math)

if(FTE_ENGINE_USE_DXVK)
	if(LINUX)
		FetchContent_Declare(dxvk
			URL "https://github.com/doitsujin/dxvk/releases/download/v3.1.1/dxvk-native-3.1.1-steamrt-sniper.tar.gz"
			URL_HASH MD5=2937ab1f2726af08dec02f24c498cbde
			EXCLUDE_FROM_ALL
		)
		FetchContent_MakeAvailable(dxvk)
	elseif(WIN32)
		FetchContent_Declare(dxvk
			URL "https://github.com/doitsujin/dxvk/releases/download/v3.1.1/dxvk-3.1.1.tar.gz"
			URL_HASH MD5=dfca4e1ee1399ebd365c1b1b919dc78a
			EXCLUDE_FROM_ALL
		)
		FetchContent_MakeAvailable(dxvk)
	else()
		message(FATAL_ERROR "DXVK is not supported for this platform")
	endif()
	list(APPEND CMAKE_PREFIX_PATH ${dxvk_SOURCE_DIR})
	if(FTE_ENGINE_RENDERER STREQUAL d3d8)
		find_library(DXVK_LIBRARY
			REQUIRED
			NAMES
				dxvk_d3d8
				libdxvk_d3d8
				d3d8
		)
	elseif(FTE_ENGINE_RENDERER STREQUAL d3d9)
		find_library(DXVK_LIBRARY
			REQUIRED
			NAMES
				dxvk_d3d9
				libdxvk_d3d9
				d3d9
		)
	elseif(FTE_ENGINE_RENDERER STREQUAL d3d11)
		find_library(DXVK_LIBRARY
			REQUIRED
			NAMES
				dxvk_d3d11
				libdxvk_d3d11
				d3d11
		)
	endif()
	list(APPEND FTE_COMMON_DEFINITIONS AVAIL_DXVK)
	if(LINUX)
		set(DXVK_INCLUDE_DIR "${dxvk_SOURCE_DIR}/usr/include/dxvk")
	endif()
endif()

if(NXDK)
	cmake_pkg_config(IMPORT zlib REQUIRED)
	set(ZLIB_LIBRARY PkgConfig::zlib)
	set(ZLIB_LIBRARIES PkgConfig::zlib)
elseif(FTE_VENDOR_DEPENDENCIES OR EMSCRIPTEN)
	FetchContent_Declare(ZLIB
		URL "https://zlib.net/zlib-1.3.2.tar.gz"
		URL_HASH MD5=a1e6c958597af3c67d162995a342138a
		EXCLUDE_FROM_ALL
		# FIND_PACKAGE_ARGS NAMES ZLIB
		OVERRIDE_FIND_PACKAGE
	)
	set(ZLIB_USE_STATIC_LIBS ON CACHE STRING "")
	set(ZLIB_BUILD_SHARED OFF CACHE STRING "")
	set(ZLIB_BUILD_STATIC ON CACHE STRING "")
	FetchContent_MakeAvailable(ZLIB)
	add_library(ZLIB::ZLIB ALIAS zlibstatic)
	set(ZLIB_INCLUDE_DIR ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})
	set(ZLIB_INCLUDE_DIRS ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})
	set(ZLIB_LIBRARY $<TARGET_FILE:zlibstatic>)
	set(ZLIB_LIBRARIES $<TARGET_FILE:zlibstatic>)
	list(APPEND FTE_COMMON_DEFINITIONS AVAIL_ZLIB)
else()
	set(ZLIB_USE_STATIC_LIBS ON)
	find_package(ZLIB)
	if(ZLIB_FOUND)
		list(APPEND FTE_COMMON_DEFINITIONS AVAIL_ZLIB)
	else()
		list(APPEND FTE_COMMON_DEFINITIONS NO_ZLIB)
		message(WARNING "zlib not found")
	endif()
endif()

if(FTE_PLUGIN_BOX3D)
	if(FTE_VENDOR_DEPENDENCIES)
		FetchContent_Declare(box3d
			GIT_REPOSITORY "https://github.com/erincatto/box3d.git"
			GIT_TAG "v0.1.0"
			EXCLUDE_FROM_ALL
			GIT_SHALLOW TRUE
			GIT_PROGRESS TRUE
			# FIND_PACKAGE_ARGS NAMES box3d
			OVERRIDE_FIND_PACKAGE
		)
		FetchContent_MakeAvailable(box3d)
	else()
		find_package(box3d 0.1 REQUIRED)
	endif()
endif()

if(FTE_PLUGIN_BULLET)
	if(FTE_VENDOR_DEPENDENCIES)
		FetchContent_Declare(Bullet
			URL "https://github.com/bulletphysics/bullet3/archive/refs/tags/2.89.tar.gz"
			URL_HASH MD5=d239b4800ec30513879834be6fcdc376
			EXCLUDE_FROM_ALL
			# FIND_PACKAGE_ARGS NAMES Bullet
			OVERRIDE_FIND_PACKAGE
		)
		set(BUILD_BULLET2_DEMOS OFF CACHE STRING "")
		set(BUILD_EXTRAS OFF CACHE STRING "")
		set(BUILD_BULLET3 OFF CACHE STRING "")
		FetchContent_MakeAvailable(Bullet)
	else()
		find_package(Bullet REQUIRED)
	endif()
endif()

if(FTE_PLUGIN_JOLT)
	FetchContent_Declare(JoltPhysics
		GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics.git"
		GIT_TAG "v5.5.0"
		SOURCE_SUBDIR "Build"
		EXCLUDE_FROM_ALL
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
	)
	FetchContent_MakeAvailable(JoltPhysics)
endif()

if(FTE_PLUGIN_ODE)
	FetchContent_Declare(ODE
		GIT_REPOSITORY "https://bitbucket.org/odedevs/ode.git"
		GIT_TAG "0.16.6"
		EXCLUDE_FROM_ALL
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
	)
	set(ODE_DOUBLE_PRECISION OFF CACHE STRING "")
	set(ODE_WITH_DEMOS OFF CACHE STRING "")
	FetchContent_MakeAvailable(ODE)
	target_compile_options(ODE
		PUBLIC
			$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU,Clang>>:-Wno-deprecated-enum-enum-conversion>
			$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU,Clang>>:-Wno-deprecated-enum-float-conversion>
			$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:MSVC>>:/wd5055>
	)
endif()

if(FTE_PLUGIN_OPENXR)
	cmake_pkg_config(IMPORT openxr)
	if(NOT PKGCONFIG_openxr_FOUND)
		set(FTE_PLUGIN_OPENXR OFF)
	endif()
endif()

if(FTE_PLUGIN_OPENSSL)
	cmake_pkg_config(IMPORT openssl)
	if(NOT PKGCONFIG_openssl_FOUND)
		set(FTE_PLUGIN_OPENSSL OFF)
	endif()
endif()

if(FTE_TOOL_HEIGHTMAPCONVERTER)
	FetchContent_Declare(inih
		GIT_REPOSITORY "https://github.com/benhoyt/inih.git"
		GIT_TAG "origin/master"
		EXCLUDE_FROM_ALL
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
	)
	FetchContent_MakeAvailable(inih)
endif()

if(FTE_ENGINE_CLIENT AND NOT EMSCRIPTEN)
	if(FTE_ENGINE_USE_SDL)
		if(FTE_ENGINE_SDL_VERSION_MAJOR STREQUAL "1")
			if(FTE_VENDOR_DEPENDENCIES)
				message(FATAL_ERROR "Vendoring SDL 1.2 is currently unsupported")
			else()
				find_package(SDL REQUIRED)
			endif()
		elseif(FTE_ENGINE_SDL_VERSION_MAJOR STREQUAL "2")
			if(FTE_VENDOR_DEPENDENCIES)
				FetchContent_Declare(SDL2
					GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
					GIT_TAG "release-2.32.10"
					EXCLUDE_FROM_ALL
					GIT_SHALLOW TRUE
					GIT_PROGRESS TRUE
					# FIND_PACKAGE_ARGS NAMES SDL2
					OVERRIDE_FIND_PACKAGE
				)
				set(SDL_TEST_LIBRARY OFF CACHE STRING "")
				set(SDL_SHARED OFF CACHE STRING "")
				set(SDL_STATIC ON CACHE STRING "")
				FetchContent_MakeAvailable(SDL2)
			else()
				find_package(SDL2 REQUIRED)
			endif()
		elseif(FTE_ENGINE_SDL_VERSION_MAJOR STREQUAL "3")
			if(FTE_VENDOR_DEPENDENCIES)
				FetchContent_Declare(SDL3
					GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
					GIT_TAG "release-3.4.16"
					EXCLUDE_FROM_ALL
					GIT_SHALLOW TRUE
					GIT_PROGRESS TRUE
					# FIND_PACKAGE_ARGS NAMES SDL3
					OVERRIDE_FIND_PACKAGE
				)
				set(SDL_TEST_LIBRARY OFF CACHE STRING "")
				set(SDL_SHARED OFF CACHE STRING "")
				set(SDL_STATIC ON CACHE STRING "")
				FetchContent_MakeAvailable(SDL3)
			else()
				find_package(SDL3 REQUIRED)
			endif()
		endif()
	endif()
	if(FTE_ENGINE_RENDERER STREQUAL "gl")
		find_package(OpenGL REQUIRED)
	elseif(FTE_ENGINE_RENDERER STREQUAL "vk")
		find_package(Vulkan REQUIRED)
	endif()
	if(FTE_VENDOR_DEPENDENCIES)
		if(FTE_ENGINE_USE_FREETYPE)
			FetchContent_Declare(Freetype
				URL "https://download.savannah.gnu.org/releases/freetype/freetype-2.14.3.tar.gz"
				URL_HASH MD5=c8333525a49e3caf08f427f1a4b01f35
				EXCLUDE_FROM_ALL
				# FIND_PACKAGE_ARGS NAMES Freetype
				OVERRIDE_FIND_PACKAGE
			)
			FetchContent_MakeAvailable(Freetype)
			list(APPEND FTE_COMMON_DEFINITIONS AVAIL_FREETYPE FREETYPE_STATIC)
		else()
			list(APPEND FTE_COMMON_DEFINITIONS NO_FREETYPE)
		endif()
	else()
		find_package(Freetype)
		if(Freetype_FOUND)
			add_library(freetype ALIAS Freetype::Freetype)
			list(APPEND FTE_COMMON_DEFINITIONS AVAIL_FREETYPE)
		else()
			message(WARNING "Freetype not found, TTF fonts will not render")
		endif()
	endif()
	if(FTE_ENGINE_USE_OGGVORBIS)
		if(FTE_VENDOR_DEPENDENCIES)
			FetchContent_Declare(Ogg
				URL "https://ftp.osuosl.org/pub/xiph/releases/ogg/libogg-1.3.6.tar.gz"
				URL_HASH MD5=e2ab08345a440d32e88b2156cf499eb9
				EXCLUDE_FROM_ALL
				# FIND_PACKAGE_ARGS NAMES Ogg
				OVERRIDE_FIND_PACKAGE
			)
			FetchContent_MakeAvailable(Ogg)
			list(APPEND CMAKE_PREFIX_PATH ${ogg_BINARY_DIR})
			set(Ogg_DIR ${ogg_BINARY_DIR})
			FetchContent_Declare(Vorbis
				URL "https://ftp.osuosl.org/pub/xiph/releases/vorbis/libvorbis-1.3.7.tar.gz"
				URL_HASH MD5=9b8034da6edc1a17d18b9bc4542015c7
				EXCLUDE_FROM_ALL
				# FIND_PACKAGE_ARGS NAMES Vorbis
				OVERRIDE_FIND_PACKAGE
			)
			FetchContent_MakeAvailable(Vorbis)
			list(APPEND FTE_COMMON_DEFINITIONS AVAIL_OGGVORBIS LIBVORBISFILE_STATIC)
		else()
			find_package(Ogg)
			find_package(Vorbis)
			if(Ogg_FOUND AND Vorbis_FOUND)
				list(APPEND FTE_COMMON_DEFINITIONS AVAIL_OGGVORBIS)
			else()
				message(WARNING "Ogg/Vorbis not found")
			endif()
		endif()
	endif()
	if(FTE_ENGINE_USE_SQL)
		if(FTE_VENDOR_DEPENDENCIES)
			FetchContent_Declare(SQLite3
				URL "https://www.sqlite.org/2026/sqlite-amalgamation-3530400.zip"
				URL_HASH MD5=9af095f7326edd65e4b2f3b3382f8cfb
				EXCLUDE_FROM_ALL
				OVERRIDE_FIND_PACKAGE
			)
			FetchContent_MakeAvailable(SQLite3)
			add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
			target_include_directories(sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})
			list(APPEND FTE_COMMON_DEFINITIONS USE_SQLITE)
		else()
			find_package(SQLite3)
			if(SQLite3_FOUND)
				add_library(sqlite3 ALIAS SQLite3::SQLite3)
				list(APPEND FTE_COMMON_DEFINITIONS USE_SQLITE)
			else()
				message(WARNING "SQLite not found, SQL databases will not be available")
				set(FTE_ENGINE_USE_SQL OFF)
			endif()
		endif()
	endif()
	if(FTE_ENGINE_USE_PNG)
		if(FTE_VENDOR_DEPENDENCIES)
			FetchContent_Declare(PNG
				URL "http://prdownloads.sourceforge.net/libpng/libpng-1.6.58.tar.gz?download"
				URL_HASH MD5=40aaee5111ff68814d57351e68f15f29
				EXCLUDE_FROM_ALL
				# FIND_PACKAGE_ARGS NAMES PNG
				OVERRIDE_FIND_PACKAGE
			)
			FetchContent_MakeAvailable(PNG)
			set(PNG_SHARED FALSE CACHE STRING "")
			set(PNG_STATIC TRUE CACHE STRING "")
			add_library(PNG::PNG ALIAS png_static)
			list(APPEND FTE_COMMON_DEFINITIONS AVAIL_PNGLIB LIBPNG_STATIC)
		else()
			find_package(PNG)
			if(PNG_FOUND)
				list(APPEND FTE_COMMON_DEFINITIONS AVAIL_PNGLIB DYNAMIC_LIBPNG)
			else()
				message(WARNING "libpng not found, PNG images will not load")
			endif()
		endif()
	endif()
	list(APPEND FTE_COMMON_DEFINITIONS AVAIL_STBI)
endif()
