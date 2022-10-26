cmake_minimum_required(VERSION 3.16)
project(Lava)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions (-DBUILD_LAVA)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    MESSAGE(STATUS "Debug mode is open")
    add_definitions (-DDEBUG -DIs_True_On)
    set (OPT_FLAGS "-O0")
else ()
    MESSAGE(STATUS "Debug mode is close")
    add_definitions (-DNDEBUG)
    set (OPT_FLAGS "-O2")
endif ()

#add graphviz
option(ENABLE_CFG "Dump CFG picture" OFF)
MESSAGE(STATUS "CFG option ${ENABLE_CFG}")
if (${ENABLE_CFG})
    if (NOT GRAPHVIZ_HOME)
        set (GRAPHVIZ_HOME "$ENV{GRAPHVIZ_HOME}")
    endif()
    if (NOT GRAPHVIZ_HOME)
        message (FATAL_ERROR "GRAPHVIZ_HOME isn't set. Use -DGRAPHVIZ_HOME=<path_to_graphviz> to specify graphviz lib")
    else()
        message (STATUS "Check for GRAPHVIZ_HOME: ${GRAPHVIZ_HOME}")
    endif()

    add_definitions (-DENABLE_CFG)
    include_directories(${GRAPHVIZ_HOME}/include/graphviz)
    link_directories(${GRAPHVIZ_HOME}/lib)
endif()

set(LAVA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(LAVA_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_compile_options(-Wall -Werror)

#set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined")
#set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined")

include_directories(${LAVA_INCLUDE_DIR})

add_subdirectory("${LAVA_SOURCE_DIR}/define")
add_subdirectory("${LAVA_SOURCE_DIR}/front")
add_subdirectory("${LAVA_SOURCE_DIR}/mid")
add_subdirectory("${LAVA_SOURCE_DIR}/opt")
add_subdirectory("${LAVA_SOURCE_DIR}/back")
add_subdirectory("${LAVA_SOURCE_DIR}/driver")


# executable
add_executable(lacc main.cpp link.cpp)

if (APPLE)
    target_link_libraries(lacc
        lava::front
        lava::definelib
        lava::mid
        lava::opt
        lava::back
        lava::driver
    )
    if (${ENABLE_CFG})
      target_link_libraries(lacc gvc cgraph cdt)
    endif()
elseif (UNIX)
    target_link_libraries(lacc
        -Wl,--start-group
        lava::front
        lava::definelib
        lava::mid
        lava::opt
        lava::back
        lava::driver
        -Wl,--end-group
    )
    if (${ENABLE_CFG})
        target_link_libraries(lacc gvc cgraph cdt)
    endif()
endif()