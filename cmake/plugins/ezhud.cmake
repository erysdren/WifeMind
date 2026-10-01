if(NOT FTE_PLUGIN_EZHUD)
	return()
endif()

fte_add_plugin(ezhud
	TITLE "EzHud Plugin"
	DESCRIPTION "Provides compat with ezquake's hud scripts."
	CATEGORY "Plugins"
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/ezhud/ezquakeisms.c
		${FTE_PLUGINS_ROOT_DIR}/ezhud/hud.c
		${FTE_PLUGINS_ROOT_DIR}/ezhud/hud_common.c
		${FTE_PLUGINS_ROOT_DIR}/ezhud/hud_editor.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
	INCLUDE_DIRECTORIES
		${FTE_PLUGINS_ROOT_DIR}
		${FTE_PLUGINS_ROOT_DIR}/ezhud
		${FTE_ENGINE_CLIENT_DIR}
	DEPENDENCIES
		$<TARGET_NAME_IF_EXISTS:Math::Math>
)
