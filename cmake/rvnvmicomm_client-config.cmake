get_filename_component(RVNVMICOMM_CLIENT_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

find_dependency(rvnvmicomm_common REQUIRED)

if(NOT TARGET rvnvmicomm_client)
	include("${RVNVMICOMM_CLIENT_CMAKE_DIR}/rvnvmicomm_client-targets.cmake")
endif()
