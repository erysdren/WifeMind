if(NOT FTE_PLUGIN_MPQ)
	return()
endif()

fte_add_plugin(mpq
	TITLE "MPQ Archive Plugin"
	DESCRIPTION "Adds support for reading .mpq files. Not very useful..."
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/mpq/blast.c
		${FTE_PLUGINS_ROOT_DIR}/mpq/fs_mpq.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
	INCLUDE_DIRECTORIES
		${FTE_PLUGINS_ROOT_DIR}
		${FTE_PLUGINS_ROOT_DIR}/mpq
		${FTE_ENGINE_CLIENT_DIR}
		${FTE_ENGINE_COMMON_DIR}
		${FTE_ENGINE_QCLIB_DIR}
	DEPENDENCIES
		${ZLIB_LIBRARIES}
)
