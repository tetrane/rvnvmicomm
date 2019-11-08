cmake_minimum_required(VERSION 3.7)
project(test)

find_package(Boost 1.49 COMPONENTS
    unit_test_framework
)

if(NOT Boost_FOUND)
  message(WARNING "Boost not found, don't build tests")
  return()
endif(NOT Boost_FOUND)

# test_start_stop_connect

add_executable(test_start_stop_connect
  test_start_stop_connect.cpp
  test_common.c
)

target_link_libraries(test_start_stop_connect
  PUBLIC
    Boost::boost

  PRIVATE
    rvnvmicomm_client
    rvnvmicomm_server
    Boost::unit_test_framework
    pthread
)

target_compile_definitions(test_start_stop_connect PRIVATE "BOOST_TEST_DYN_LINK")

add_test(rvnvmicomm::start_stop_connect test_start_stop_connect)

# test_read_memory

add_executable(test_read_memory
  test_read_memory.cpp
  test_common.c
)

target_link_libraries(test_read_memory
  PUBLIC
    Boost::boost

  PRIVATE
    rvnvmicomm_client
    rvnvmicomm_server
    Boost::unit_test_framework
    pthread
)

target_compile_definitions(test_read_memory PRIVATE "BOOST_TEST_DYN_LINK")

add_test(rvnvmicomm::test_read_memory test_read_memory)

# test_read_register

add_executable(test_read_register
  test_read_register.cpp
  test_common.c
)

target_link_libraries(test_read_register
  PUBLIC
    Boost::boost

  PRIVATE
    rvnvmicomm_client
    rvnvmicomm_server
    Boost::unit_test_framework
    pthread
)

target_compile_definitions(test_read_register PRIVATE "BOOST_TEST_DYN_LINK")

add_test(rvnvmicomm::test_read_register test_read_register)
