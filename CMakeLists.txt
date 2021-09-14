cmake_minimum_required(VERSION 3.7)
project(rvnvmicomm)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

option(BUILD_SHARED_LIBS "Set to ON to build shared libraries; OFF for static libraries." OFF)
option(WARNING_AS_ERROR "Set to ON to build with -Werror" ON)

option(BUILD_TEST_COVERAGE "Set to ON to build while generating coverage information. Will put source on the build directory." OFF)

include(GNUInstallDirs)

##################
# Common
##################

add_library(rvnvmicomm_common INTERFACE)

target_include_directories(rvnvmicomm_common
  INTERFACE
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/rvnvmicomm_common DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS rvnvmicomm_common
  EXPORT rvnvmicomm_common-export
)

install(EXPORT rvnvmicomm_common-export
  FILE
    rvnvmicomm_common-config.cmake
  DESTINATION
    ${CMAKE_INSTALL_DATADIR}/cmake/rvnvmicomm_common
)

##################
# Client
##################

add_library(rvnvmicomm_client STATIC
  src/client/vmiclient.c
)

target_compile_options(rvnvmicomm_client PRIVATE -W -Wall -Wextra -Wmissing-include-dirs -Wunknown-pragmas -Wpointer-arith -Wmissing-field-initializers -Wno-multichar -Wreturn-type)

if(WARNING_AS_ERROR)
  target_compile_options(rvnvmicomm_client PRIVATE -Werror)
endif()

if(BUILD_TEST_COVERAGE)
  target_compile_options(rvnvmicomm_client PRIVATE -g -O0 --coverage -fprofile-arcs -ftest-coverage)
  target_link_libraries(rvnvmicomm_client PRIVATE gcov)
endif()

target_include_directories(rvnvmicomm_client
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
)

target_link_libraries(rvnvmicomm_client
  PUBLIC
    rvnvmicomm_common
)

set(PUBLIC_HEADERS
  include/rvnvmicomm_client/vmiclient.h
)

set_target_properties(rvnvmicomm_client PROPERTIES
  PUBLIC_HEADER "${PUBLIC_HEADERS}"
  POSITION_INDEPENDENT_CODE ON
)

install(TARGETS rvnvmicomm_client
  EXPORT rvnvmicomm_client-export
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rvnvmicomm_client
)

install(EXPORT rvnvmicomm_client-export
  FILE
    rvnvmicomm_client-targets.cmake
  DESTINATION
    ${CMAKE_INSTALL_DATADIR}/cmake/rvnvmicomm_client
)

install(FILES cmake/rvnvmicomm_client-config.cmake
  DESTINATION ${CMAKE_INSTALL_DATADIR}/cmake/rvnvmicomm_client
)

##################
# Server
##################

add_library(rvnvmicomm_server
  src/server/vmiserver.c
)

target_compile_options(rvnvmicomm_server PRIVATE -W -Wall -Wextra -Wmissing-include-dirs -Wunknown-pragmas -Wpointer-arith -Wmissing-field-initializers -Wno-multichar -Wreturn-type)

if(WARNING_AS_ERROR)
  target_compile_options(rvnvmicomm_server PRIVATE -Werror)
endif()

if(BUILD_TEST_COVERAGE)
  target_compile_options(rvnvmicomm_server PRIVATE -g -O0 --coverage -fprofile-arcs -ftest-coverage)
  target_link_libraries(rvnvmicomm_server PRIVATE gcov)
endif()

target_include_directories(rvnvmicomm_server
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
)

target_link_libraries(rvnvmicomm_server
  PUBLIC
    rvnvmicomm_common
)

set(PUBLIC_HEADERS
  include/rvnvmicomm_server/vmiserver.h
)

set_target_properties(rvnvmicomm_server PROPERTIES
  PUBLIC_HEADER "${PUBLIC_HEADERS}"
  POSITION_INDEPENDENT_CODE ON
)

install(TARGETS rvnvmicomm_server
  EXPORT rvnvmicomm_server-export
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rvnvmicomm_server
)

install(EXPORT rvnvmicomm_server-export
  FILE
    rvnvmicomm_server-targets.cmake
  DESTINATION
    ${CMAKE_INSTALL_DATADIR}/cmake/rvnvmicomm_server
)

install(FILES cmake/rvnvmicomm_server-config.cmake
  DESTINATION ${CMAKE_INSTALL_DATADIR}/cmake/rvnvmicomm_server
)

enable_testing()
add_subdirectory(test)
