cmake_minimum_required(VERSION 3.16)
project(Lava)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions (-DBUILD_LAVA)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions (-DDEBUG -DIs_True_On)
    set (OPT_FLAGS "-O0")
else ()
    add_definitions (-DNDEBUG)
    set (OPT_FLAGS "-O2")
endif ()

set(LAVA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(LAVA_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_compile_options(-Wall -Werror)

include_directories(${LAVA_INCLUDE_DIR})

file(GLOB_RECURSE SOURCES "${LAVA_SOURCE_DIR}/*.cpp")
#add_subdirectory("${LAVA_SOURCE_DIR}/define")
#add_subdirectory("${LAVA_SOURCE_DIR}/front")
#add_subdirectory("${LAVA_SOURCE_DIR}/mid")
#add_subdirectory("${LAVA_SOURCE_DIR}/opt")
#add_subdirectory("${LAVA_SOURCE_DIR}/driver")

# executable
add_executable(lacc main.cpp link.cpp ${SOURCES})
#add_executable(lacc main.cpp link.cpp)
#
#target_link_libraries(lacc
#        lava::definelib
#        lava::front
#        lava::mid
#        lava::opt
#        lava::driver
#)