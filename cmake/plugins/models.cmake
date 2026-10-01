if(NOT FTE_PLUGIN_MODELS)
	return()
endif()

fte_add_plugin(models
	TITLE "Models Plugin"
	DESCRIPTION "Kinda redundant now that the engine has gltf2 loading."
	CATEGORY "Plugins"
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/models/models.c
		${FTE_PLUGINS_ROOT_DIR}/models/gltf.c
		# ${DRACO_FILES}
		${FTE_ENGINE_COMMON_DIR}/json.c
		${FTE_PLUGINS_ROOT_DIR}/models/exportiqm.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
	INCLUDE_DIRECTORIES
		${FTE_PLUGINS_ROOT_DIR}
		${FTE_PLUGINS_ROOT_DIR}/models
		${FTE_ENGINE_CLIENT_DIR}
		${FTE_ENGINE_QCLIB_DIR}
		${FTE_ENGINE_COMMON_DIR}
		${FTE_ENGINE_GL_DIR}
	DEPENDENCIES
		$<TARGET_NAME_IF_EXISTS:Math::Math>
		# ${DRACO_LIBRARY}
)
