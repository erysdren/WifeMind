if(NOT FTE_PLUGINS)
	return()
endif()

function(fte_add_plugin name)
	cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES")
	add_library(${name} SHARED ${ARG_SOURCES})
	target_compile_options(${name} PRIVATE ${FTE_COMMON_OPTIONS})
	target_compile_definitions(${name} PRIVATE FTEPLUGIN ${FTE_COMMON_DEFINITIONS})
	set_target_properties(${name}
		PROPERTIES
			LIBRARY_OUTPUT_DIRECTORY $<1:${FTE_INSTALL_PREFIX}/bin>
			RUNTIME_OUTPUT_DIRECTORY $<1:${FTE_INSTALL_PREFIX}/bin>
			SUFFIX ${FTE_SHARED_LIBRARY_SUFFIX}
			PREFIX ""
	)
endfunction()

file(GLOB plugins "${PROJECT_SOURCE_DIR}/cmake/plugins/*.cmake")
foreach(plugin IN LISTS plugins)
	include(${plugin})
endforeach()
