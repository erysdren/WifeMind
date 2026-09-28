
if(NOT CMAKE_SYSTEM_PROCESSOR)
	set(FTE_SYSTEM_PROCESSOR "unknown")
else()
	string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} _system_processor)
	if(_system_processor STREQUAL "amd64" OR _system_processor STREQUAL "x64")
		set(_system_processor "x86_64")
	endif()
	set(FTE_SYSTEM_PROCESSOR ${_system_processor})
endif()

if(EMSCRIPTEN)
	set(FTE_EXECUTABLE_SUFFIX ".js")
else()
	set(FTE_EXECUTABLE_SUFFIX ".${FTE_SYSTEM_PROCESSOR}${CMAKE_EXECUTABLE_SUFFIX}")
endif()

set(FTE_SHARED_LIBRARY_SUFFIX ".${FTE_SYSTEM_PROCESSOR}${CMAKE_SHARED_LIBRARY_SUFFIX}")
