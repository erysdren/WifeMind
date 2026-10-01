if(NOT FTE_PLUGIN_HL2)
	return()
endif()

fte_add_plugin(hl2
	TITLE "HalfLife2 Formats Plugin"
	DESCRIPTION "Adds support for reading various file formats used by HalfLife2. Requires mod support to be useful."
	CATEGORY "Plugins"
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/hl2/fs_vpk.c
		${FTE_PLUGINS_ROOT_DIR}/hl2/fs_vpk_vtmb.c
		${FTE_PLUGINS_ROOT_DIR}/hl2/fs_gma.c
		${FTE_PLUGINS_ROOT_DIR}/hl2/img_tth.c
		${FTE_PLUGINS_ROOT_DIR}/hl2/img_vtf.c
		${FTE_PLUGINS_ROOT_DIR}/hl2/mod_hl2.c
		${FTE_PLUGINS_ROOT_DIR}/hl2/mat_vmt.c
		${FTE_PLUGINS_ROOT_DIR}/hl2/mod_vbsp.c
		${FTE_PLUGINS_ROOT_DIR}/hl2/hl2.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
	INCLUDE_DIRECTORIES
		${FTE_PLUGINS_ROOT_DIR}
		${FTE_PLUGINS_ROOT_DIR}/hl2
		${FTE_ENGINE_CLIENT_DIR}
		${FTE_ENGINE_QCLIB_DIR}
		${FTE_ENGINE_GL_DIR}
		${FTE_ENGINE_COMMON_DIR}
	DEPENDENCIES
		$<TARGET_NAME_IF_EXISTS:Math::Math>
		$<TARGET_NAME_IF_EXISTS:zlibstatic>
	COMPILE_DEFINITIONS
		MULTITHREAD
)
