cmake_minimum_required(VERSION 3.7)
project(test)

find_package(Boost 1.49 COMPONENTS
    unit_test_framework
)

if(NOT Boost_FOUND)
  message(WARNING "Boost not found, don't build tests")
  return()
endif(NOT Boost_FOUND)

add_executable(test_main
  test_main.cpp
)

target_link_libraries(test_main
  PUBLIC
    Boost::boost

  PRIVATE
    rvnvmicomm_client
    rvnvmicomm_server
    Boost::unit_test_framework
)

target_compile_definitions(test_main PRIVATE "BOOST_TEST_DYN_LINK")

add_test(rvnvmicomm::main test_main)
