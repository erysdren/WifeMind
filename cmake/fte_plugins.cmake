if(NOT FTE_PLUGINS)
	return()
endif()

function(fte_add_plugin name)
	cmake_parse_arguments(PARSE_ARGV 1 ARG "" "TITLE;DESCRIPTION;GAMEDIR;CATEGORY" "SOURCES;INCLUDE_DIRECTORIES;DEPENDENCIES;COMPILE_DEFINITIONS")
	set(target fteplug_${name})
	add_library(${target} SHARED ${ARG_SOURCES})
	fte_add_common(${target})
	target_link_options(${target}
		PRIVATE
			$<$<C_COMPILER_ID:GNU,Clang>:-Wl,--no-undefined>
	)
	target_link_libraries(${target}
		PRIVATE
			${ARG_DEPENDENCIES}
	)
	target_compile_definitions(${target}
		PRIVATE
			FTEPLUGIN
			${ARG_COMPILE_DEFINITIONS}
	)
	target_include_directories(${target}
		PRIVATE
			${ARG_INCLUDE_DIRECTORIES}
	)
	set_target_properties(${target}
		PROPERTIES
			LIBRARY_OUTPUT_DIRECTORY $<1:${FTE_INSTALL_PREFIX}>
			RUNTIME_OUTPUT_DIRECTORY $<1:${FTE_INSTALL_PREFIX}>
			SUFFIX ${FTE_SHARED_LIBRARY_SUFFIX}
			PREFIX ""
			FTEPLUG_NAME "fte${name}"
			FTEPLUG_TITLE "${ARG_TITLE}"
			FTEPLUG_DESCRIPTION "${ARG_DESCRIPTION}"
			FTEPLUG_VERSION "${FTE_SVNREVISION}"
			FTEPLUG_GAMEDIR "${ARG_GAMEDIR}"
			FTEPLUG_CATEGORY "${ARG_CATEGORY}"
	)
	set(temp_dir "${PROJECT_BINARY_DIR}/${target}.d/")
	set(info_file "-")
	set(zip_file "${target}.zip")
	file(MAKE_DIRECTORY "${temp_dir}")
	add_custom_command(
		TARGET ${target} POST_BUILD
		WORKING_DIRECTORY "${temp_dir}"
		COMMAND ${CMAKE_COMMAND} ARGS -E echo
[[{
	package $<TARGET_PROPERTY:FTEPLUG_NAME>
	ver "$<TARGET_PROPERTY:FTEPLUG_VERSION>"
	category "$<TARGET_PROPERTY:FTEPLUG_CATEGORY>"
	title "$<TARGET_PROPERTY:FTEPLUG_TITLE>"
	gamedir "$<TARGET_PROPERTY:FTEPLUG_GAMEDIR>"
	desc "$<TARGET_PROPERTY:FTEPLUG_DESCRIPTION>"
}]] > "${info_file}"
		COMMAND ${CMAKE_COMMAND} ARGS -E tar c "${zip_file}" --format=zip --cmake-tar-compression-method=store "${info_file}"
		COMMAND ${CMAKE_COMMAND} ARGS -E cat "${zip_file}" >> "$<TARGET_FILE:${target}>"
		VERBATIM
	)
endfunction()

file(GLOB plugins "${PROJECT_SOURCE_DIR}/cmake/plugins/*.cmake")
foreach(plugin IN LISTS plugins)
	include(${plugin})
endforeach()
