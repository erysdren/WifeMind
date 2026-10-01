if(NOT FTE_PLUGIN_COD)
	return()
endif()

fte_add_plugin(cod
	TITLE "CoD Formats"
	DESCRIPTION "Provides compatability with Call Of Duty's file formats."
	CATEGORY "Plugins"
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/cod/codmod.c
		${FTE_PLUGINS_ROOT_DIR}/cod/codbsp.c
		${FTE_PLUGINS_ROOT_DIR}/cod/codmat.c
		${FTE_PLUGINS_ROOT_DIR}/cod/codiwi.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
	INCLUDE_DIRECTORIES
		${FTE_PLUGINS_ROOT_DIR}
		${FTE_PLUGINS_ROOT_DIR}/cod
		${FTE_ENGINE_CLIENT_DIR}
		${FTE_ENGINE_QCLIB_DIR}
		${FTE_ENGINE_GL_DIR}
	DEPENDENCIES
		$<TARGET_NAME_IF_EXISTS:Math::Math>
	COMPILE_DEFINITIONS
		MULTITHREAD
)
