cmake_minimum_required(VERSION 2.6)
project(bsp2js C)

find_package(JPEG REQUIRED)

include_directories(${JPEG_INCLUDE_DIR})

add_executable(bsp2js bsp2js.c)
target_link_libraries(bsp2js ${JPEG_LIBRARY})
