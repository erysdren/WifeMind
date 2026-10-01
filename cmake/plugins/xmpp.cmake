if(NOT FTE_PLUGIN_XMPP)
	return()
endif()

fte_add_plugin(xmpp
	TITLE "XMPP Plugin"
	DESCRIPTION "XMPP/Jabber instant messenger plugin for chatting without tabbing out."
	CATEGORY "Plugins"
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/jabber/jabberclient.c
		${FTE_PLUGINS_ROOT_DIR}/jabber/jingle.c
		${FTE_PLUGINS_ROOT_DIR}/jabber/sift.c
		${FTE_PLUGINS_ROOT_DIR}/jabber/xml.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
		${FTE_ENGINE_COMMON_DIR}/sha1.c
		${FTE_ENGINE_COMMON_DIR}/sha2.c
		${FTE_PLUGINS_ROOT_DIR}/emailnot/md5.c
	INCLUDE_DIRECTORIES
		${FTE_PLUGINS_ROOT_DIR}
		${FTE_PLUGINS_ROOT_DIR}/jabber
		${FTE_ENGINE_CLIENT_DIR}
)
