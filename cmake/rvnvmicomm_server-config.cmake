get_filename_component(RVNVMICOMM_SERVER_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

find_dependency(rvnvmicomm_common REQUIRED)

if(NOT TARGET rvnvmicomm_server)
	include("${RVNVMICOMM_SERVER_CMAKE_DIR}/rvnvmicomm_server-targets.cmake")
endif()
