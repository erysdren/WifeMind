if(NOT FTE_PLUGIN_OPENSSL)
	return()
endif()

fte_add_plugin(openssl
	TITLE "OpenSSL"
	DESCRIPTION "Provides OpenSSL support for dtls/tls/https support. The crypto library that is actually used is controlled via the tls_provider cvar."
	SOURCES
		${FTE_PLUGINS_ROOT_DIR}/net_ssl_openssl.c
		${FTE_PLUGINS_ROOT_DIR}/plugin.c
	INCLUDE_DIRECTORIES
		${FTE_PLUGINS_ROOT_DIR}
		${FTE_ENGINE_CLIENT_DIR}
		${FTE_ENGINE_COMMON_DIR}
	DEPENDENCIES
		PkgConfig::openssl
)
