cmake_minimum_required(VERSION 3.7)
project(test)

find_package(Boost 1.49 COMPONENTS
    unit_test_framework
    filesystem
)

if(NOT Boost_FOUND)
  message(WARNING "Boost not found, don't build tests")
  return()
endif(NOT Boost_FOUND)

add_executable(test_callbacks
  test_callbacks.cpp
  test_server_implem.c
)

target_link_libraries(test_callbacks
  PUBLIC
    Boost::boost
    Boost::filesystem

  PRIVATE
    rvnvmicomm_client
    rvnvmicomm_server
    Boost::unit_test_framework
    pthread
)

target_compile_definitions(test_callbacks PRIVATE "BOOST_TEST_DYN_LINK")

add_test(rvnvmicomm::start_stop_connect test_callbacks)
