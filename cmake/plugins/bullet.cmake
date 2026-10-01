if(NOT FTE_PLUGIN_BULLET)
	return()
endif()

fte_add_plugin(bullet
	TITLE "Bullet Physics Plugin"
	DESCRIPTION "Provides Rigid Body Physics."
	CATEGORY "Plugins"
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/bullet/bulletplug.cpp
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
	INCLUDE_DIRECTORIES
		${FTE_PLUGINS_ROOT_DIR}
		${FTE_PLUGINS_ROOT_DIR}/bullet
		${FTE_ENGINE_COMMON_DIR}
		${FTE_ENGINE_CLIENT_DIR}
		${FTE_ENGINE_QCLIB_DIR}
		${FTE_ENGINE_GL_DIR}
		${bullet_SOURCE_DIR}/src
	DEPENDENCIES
		BulletDynamics
		BulletCollision
		LinearMath
)
