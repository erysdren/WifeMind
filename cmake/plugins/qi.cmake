if(NOT FTE_PLUGIN_QI)
	return()
endif()

fte_add_plugin(qi
	TITLE "Quaddicted Map Database"
	DESCRIPTION "Provides easy access to the quaddicted map database. Once installed you can use eg 'map qi_dopa:start' to begin playing dopa, or load it via the menus."
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/qi/qi.c
		${FTE_PLUGINS_ROOT_DIR}/jabber/xml.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
	INCLUDE_DIRECTORIES
		${FTE_PLUGINS_ROOT_DIR}
		${FTE_PLUGINS_ROOT_DIR}/qi
		${FTE_ENGINE_QCLIB_DIR}
		${FTE_ENGINE_COMMON_DIR}
		${FTE_ENGINE_CLIENT_DIR}
)
