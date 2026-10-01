if(NOT FTE_PLUGIN_TERRAINGEN)
	return()
endif()

fte_add_plugin(terraingen
	TITLE "TerrainGen Plugin"
	DESCRIPTION "A lame example plugin for randomised terrain generation."
	CATEGORY "Plugins"
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/terrorgen/terragen.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
	INCLUDE_DIRECTORIES
		${FTE_PLUGINS_ROOT_DIR}
		${FTE_PLUGINS_ROOT_DIR}/terrorgen
		${FTE_ENGINE_QCLIB_DIR}
		${FTE_ENGINE_COMMON_DIR}
		${FTE_ENGINE_GL_DIR}
		${FTE_ENGINE_CLIENT_DIR}
)
