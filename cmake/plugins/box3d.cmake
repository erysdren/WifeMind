if(NOT FTE_PLUGIN_BOX3D)
	return()
endif()

fte_add_plugin(box3d
	TITLE "Box3D Physics Plugin"
	DESCRIPTION "Adds a rigidbody physics engine."
	CATEGORY "Plugins"
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/box3d/box3d.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
	INCLUDE_DIRECTORIES
		${FTE_PLUGINS_ROOT_DIR}
		${FTE_PLUGINS_ROOT_DIR}/box3d
		${FTE_ENGINE_COMMON_DIR}
		${FTE_ENGINE_CLIENT_DIR}
		${FTE_ENGINE_QCLIB_DIR}
		${FTE_ENGINE_GL_DIR}
	DEPENDENCIES
		box3d::box3d
)
