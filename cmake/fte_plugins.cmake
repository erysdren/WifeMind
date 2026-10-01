if(NOT FTE_PLUGINS)
	return()
endif()

if(0)
FUNCTION(EMBED_PLUGIN_META PLUGNAME PLUGTITLE PLUGDESC)
	SET_TARGET_PROPERTIES(${target} PROPERTIES OUTPUT_NAME "${PLUGNAME}")
	SET_TARGET_PROPERTIES(${target} PROPERTIES PREFIX "fteplug_")
	SET_TARGET_PROPERTIES(${target} PROPERTIES LINK_FLAGS "-Wl,--no-undefined")
	SET(INSTALLTARGS ${INSTALLTARGS} "${target}" PARENT_SCOPE)
	#sadly we need to use a temp zip file, because otherwise zip insists on using zip64 extensions which breaks zip -A (as well as any attempts to read any files).
	ADD_CUSTOM_COMMAND(
		TARGET ${target} POST_BUILD
		COMMAND /bin/echo -e "{\\n	package fte${target}\\n	ver \"${SVNREVISION}\"\\n	category Plugins\\n	title \"${PLUGTITLE}\"\\n	gamedir \"\"\\n	desc \"${PLUGDESC}\"\\n}" | zip -q -9 -fz- $<TARGET_FILE:${target}>.zip -
		COMMAND cmake -E cat $<TARGET_FILE:${target}>.zip >> "$<TARGET_FILE:${target}>"
		COMMAND zip -A "$<TARGET_FILE:${target}>"
		COMMAND cmake -E rm $<TARGET_FILE:${target}>.zip
		VERBATIM)
ENDFUNCTION()
endif()

function(fte_add_plugin name)
	cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES;INCLUDE_DIRECTORIES;DEPENDENCIES;COMPILE_DEFINITIONS")
	set(target fteplug_${name})
	add_library(${target} SHARED ${ARG_SOURCES})
	target_compile_options(${target}
		PRIVATE
			${FTE_COMMON_OPTIONS}
	)
	target_link_options(${target}
		PRIVATE
			$<$<C_COMPILER_ID:GNU,Clang>:-Wl,--no-undefined>
			${FTE_COMMON_LINK_OPTIONS}
	)
	target_link_libraries(${target}
		PRIVATE
			${ARG_DEPENDENCIES}
	)
	target_compile_definitions(${target}
		PRIVATE
			FTEPLUGIN
			${FTE_COMMON_DEFINITIONS}
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
			TITLE "title"
			DESCRIPTION "description"
	)
	add_custom_command(
		TARGET ${target} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E echo
[[{
	package $<TARGET_PROPERTY:NAME>
	ver "${FTE_SVNREVISION}"
	category Plugins
	title "$<TARGET_PROPERTY:TITLE>"
	gamedir ""
	desc "$<TARGET_PROPERTY:DESCRIPTION>"
}]] > "$<TARGET_FILE:${target}>.info"
		COMMAND ${CMAKE_COMMAND} -E rm "$<TARGET_FILE:${target}>.info"
		VERBATIM
	)
endfunction()

file(GLOB plugins "${PROJECT_SOURCE_DIR}/cmake/plugins/*.cmake")
foreach(plugin IN LISTS plugins)
	include(${plugin})
endforeach()
