if(NOT FTE_PLUGIN_NARBDROP)
	return()
endif()

fte_add_plugin(narbdrop
	TITLE "NarbDrop Plugin"
	DESCRIPTION "Support for Narbacular Drop formats."
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/narbdrop/fs_ore.c
		${FTE_PLUGINS_ROOT_DIR}/narbdrop/mod_cmf.c
		${FTE_PLUGINS_ROOT_DIR}/narbdrop/narbdrop.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
	INCLUDE_DIRECTORIES
		${FTE_PLUGINS_ROOT_DIR}
		${FTE_PLUGINS_ROOT_DIR}/narbdrop
		${FTE_ENGINE_CLIENT_DIR}
		${FTE_ENGINE_QCLIB_DIR}
		${FTE_ENGINE_GL_DIR}
		${FTE_ENGINE_COMMON_DIR}
	DEPENDENCIES
		$<TARGET_NAME_IF_EXISTS:Math::Math>
	COMPILE_DEFINITIONS
		MULTITHREAD
)
