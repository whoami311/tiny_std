cmake_minimum_required(VERSION 3.5)

project(tiny_std)

set(CMAKE_CXX_STANDARD 17)

aux_source_directory(src/ SRC_DIR)

add_executable(tiny_std ${SRC_DIR})

target_include_directories(tiny_std PRIVATE
incl
)

include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0 # or a later release
)

FetchContent_MakeAvailable(Catch2)

add_executable(test_unique_ptr
test/test_unique_ptr.cpp
)

target_link_libraries(test_unique_ptr PRIVATE
# Catch2::Catch2
Catch2::Catch2WithMain
)

target_include_directories(test_unique_ptr PRIVATE
incl
)

enable_testing()

add_test(NAME test_unique_ptr COMMAND test_unique_ptr)
