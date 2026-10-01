if(NOT FTE_PLUGIN_WINAMP)
	return()
endif()

fte_add_plugin(winamp
	TITLE "Winamp Plugin"
	DESCRIPTION "Plugin for controlling Winamp without tabbing out."
	CATEGORY "Plugins"
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/winamp/winamp.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
	INCLUDE_DIRECTORIES
		${FTE_PLUGINS_ROOT_DIR}
		${FTE_PLUGINS_ROOT_DIR}/winamp
		${FTE_ENGINE_CLIENT_DIR}
)
