if(NOT FTE_PLUGIN_NAMEMAKER)
	return()
endif()

fte_add_plugin(namemaker
	TITLE "Name Maker Plugin"
	DESCRIPTION "Provides a lame UI for selecting arbitrary non-ascii glyphs as part of your nickname."
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/namemaker/namemaker.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
	INCLUDE_DIRECTORIES
		${FTE_PLUGINS_ROOT_DIR}
		${FTE_PLUGINS_ROOT_DIR}/namemaker
		${FTE_ENGINE_CLIENT_DIR}
)
