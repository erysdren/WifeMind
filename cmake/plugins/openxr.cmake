if(NOT FTE_PLUGIN_OPENXR)
	return()
endif()

fte_add_plugin(openxr
	TITLE "OpenXR Plugin"
	DESCRIPTION "Provides support for Virtual Reality headsets and input devices."
	CATEGORY "Plugins"
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/openxr.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
	INCLUDE_DIRECTORIES
		${FTE_PLUGINS_ROOT_DIR}
		${FTE_ENGINE_CLIENT_DIR}
	DEPENDENCIES
		$<TARGET_NAME_IF_EXISTS:Math::Math>
		PkgConfig::openxr
)
