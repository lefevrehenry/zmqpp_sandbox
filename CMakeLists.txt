cmake_minimum_required(VERSION 3.2)
project(zmqpp_test)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/")

## ZeroMQ dependency
find_package(ZMQ MODULE QUIET)
if(NOT ZMQ_FOUND)
    find_package(ZMQ CONFIG REQUIRED)
endif()
get_target_property(_ZMQ_DIR ZMQ INTERFACE_INCLUDE_DIRECTORIES)
message("-- Found ZMQ: ${_ZMQ_DIR}")

## ZeroMQ C++ bindings dependency
find_package(ZMQPP MODULE QUIET)
if(NOT ZMQPP_FOUND)
    find_package(ZMQPP CONFIG REQUIRED)
endif()
get_target_property(_ZMQPP_DIR ZMQPP INTERFACE_INCLUDE_DIRECTORIES)
message("-- Found ZMQPP: ${_ZMQPP_DIR}")

## Threads dependency
find_package(Threads REQUIRED)

add_executable(main main.cpp)
add_executable(ping ping.cpp)
add_executable(poll poll.cpp)
add_executable(tast test.cpp)
add_executable(tist test_2.cpp)
add_executable(connexion connexion.cpp)
add_executable(many_client many_client.cpp)
add_executable(engine engine.cpp)

target_link_libraries(main PRIVATE ZMQ ZMQPP Threads::Threads)
target_link_libraries(poll PRIVATE ZMQ ZMQPP Threads::Threads)
target_link_libraries(ping PRIVATE ZMQ ZMQPP Threads::Threads)
target_link_libraries(tast PRIVATE ZMQ ZMQPP Threads::Threads)
target_link_libraries(tist PRIVATE ZMQ ZMQPP Threads::Threads)
target_link_libraries(connexion PRIVATE ZMQ ZMQPP Threads::Threads)
target_link_libraries(many_client PRIVATE ZMQ ZMQPP Threads::Threads)
target_link_libraries(engine PRIVATE ZMQ ZMQPP Threads::Threads)

target_include_directories(tist PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(many_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
