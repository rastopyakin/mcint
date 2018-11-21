cmake_minimum_required(VERSION 3.8)

project(mcint LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)

find_package(Threads REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)
link_libraries(${CMAKE_THREAD_LIBS_INIT})

add_subdirectory(examples)
add_subdirectory(tests)
