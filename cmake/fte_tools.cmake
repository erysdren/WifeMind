if(NOT FTE_TOOLS)
	return()
endif()

function(fte_add_tool name)
	cmake_parse_arguments(PARSE_ARGV 1 ARG "WIN32" "" "SOURCES")
	if(ARG_WIN32)
		add_executable(${name} WIN32 ${ARG_SOURCES})
	else()
		add_executable(${name} ${ARG_SOURCES})
	endif()
	target_compile_options(${name}
		PRIVATE
			${FTE_COMMON_OPTIONS}
	)
	target_link_options(${name}
		PRIVATE
			${FTE_COMMON_LINK_OPTIONS}
	)
	target_compile_definitions(${name}
		PRIVATE
			${FTE_COMMON_DEFINITIONS}
	)
	set_target_properties(${name}
		PROPERTIES
			LIBRARY_OUTPUT_DIRECTORY $<1:${FTE_INSTALL_PREFIX}/bin>
			RUNTIME_OUTPUT_DIRECTORY $<1:${FTE_INSTALL_PREFIX}/bin>
			SUFFIX ${FTE_EXECUTABLE_SUFFIX}
	)
endfunction()

file(GLOB tools "${PROJECT_SOURCE_DIR}/cmake/tools/*.cmake")
foreach(tool IN LISTS tools)
	include(${tool})
endforeach()
