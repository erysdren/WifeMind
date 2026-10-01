if(NOT FTE_PLUGIN_IRC)
	return()
endif()

fte_add_plugin(irc
	TITLE "IRC Plugin"
	DESCRIPTION "Allows you to chat on IRC without tabbing out."
	CATEGORY "Plugins"
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/irc/ircclient.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
	INCLUDE_DIRECTORIES
		${FTE_PLUGINS_ROOT_DIR}
		${FTE_PLUGINS_ROOT_DIR}/irc
		${FTE_ENGINE_CLIENT_DIR}
)
