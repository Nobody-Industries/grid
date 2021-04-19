cmake_minimum_required(VERSION 3.7)

project(grid C)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
add_executable(grid main)
target_link_libraries(grid ${SDL2_LIBRARIES})

install(TARGETS grid RUNTIME DESTINATION .)
