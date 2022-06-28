cmake_minimum_required(VERSION 3.14)

project(os-hw3)

Include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.0.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)

set(SOURCE_DIR ${CMAKE_SOURCE_DIR})

add_executable(malloc_3_test malloc_3_test_basic.cpp malloc_3.cpp)

target_link_libraries(malloc_3_test PRIVATE Catch2::Catch2WithMain)

target_compile_options(malloc_3_test PRIVATE -Wpointer-arith )
