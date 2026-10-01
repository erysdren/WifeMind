if(NOT FTE_PLUGIN_XSV)
	return()
endif()

fte_add_plugin(xsv
	TITLE "X11 Server Plugin"
	DESCRIPTION "Provides a primitive X11 server in the form of a video decoder plugin."
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/xsv/m_x.c
		${FTE_PLUGINS_ROOT_DIR}/xsv/x_reqs.c
		${FTE_PLUGINS_ROOT_DIR}/xsv/x_res.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
		${FTE_ENGINE_QCLIB_DIR}/hash.c
	INCLUDE_DIRECTORIES
		${FTE_PLUGINS_ROOT_DIR}
		${FTE_PLUGINS_ROOT_DIR}/xsv
		${FTE_ENGINE_CLIENT_DIR}
	DEPENDENCIES
		$<TARGET_NAME_IF_EXISTS:Math::Math>
)
