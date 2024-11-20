project(tiny_std)

cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)

aux_source_directory(src/ SRC_DIR)

add_executable(tiny_std ${SRC_DIR})

target_include_directories(tiny_std PRIVATE
incl
)
