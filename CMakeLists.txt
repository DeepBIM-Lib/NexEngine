cmake_minimum_required(VERSION 4.1)

set(NexEngine_VERSION_MAJOR 0)
set(NexEngine_VERSION_MINOR 0)
set(NexEngine_VERSION_PATCH 1)
set(NexEngine_VERSION "${NexEngine_VERSION_MAJOR}.${NexEngine_VERSION_MINOR}.${NexEngine_VERSION_PATCH}")

message(STATUS "NexEngine V${NexEngine_VERSION}")

project(NexEngine VERSION ${NexEngine_VERSION} LANGUAGES C CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/MP)
    # Force source and execution character sets to UTF-8 in MSVC.
    add_compile_options($<$<COMPILE_LANGUAGE:C>:/utf-8>)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/utf-8>)
endif ()

set(Engine_Root ${CMAKE_CURRENT_SOURCE_DIR})

option(Build_GLFW_Host "Build glfw host for NexEngine" ON)

# add third-party module
add_subdirectory(externals/DiligentCore SYSTEM)
add_subdirectory(externals/DiligentTools SYSTEM)

set(FLECS_STATIC ON CACHE BOOL "Build Flecs static library")
set(FLECS_SHARED OFF CACHE BOOL "Disable Flecs shared library")
add_subdirectory(externals/flecs SYSTEM)
target_compile_definitions(flecs_static PUBLIC FLECS_SCRIPT_MATH)

if(Build_GLFW_Host)
    set(GLFW_LIBRARY_TYPE STATIC)
    set(GLFW_BUILD_DOCS OFF)
    set(GLFW_BUILD_TESTS OFF)
    set(GLFW_BUILD_EXAMPLES OFF)
    set(GLFW_INSTALL OFF)
    add_subdirectory(externals/glfw SYSTEM)
endif ()

# set build postfix
set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
set(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_RELWITHDEBINFO_POSTFIX "rd" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_MINSIZEREL_POSTFIX "s" CACHE STRING "add a postfix, usually empty on windows")

if (CMAKE_BUILD_TYPE MATCHES "Release")
    set(CMAKE_BUILD_POSTFIX "${CMAKE_RELEASE_POSTFIX}")
elseif (CMAKE_BUILD_TYPE MATCHES "MinSizeRel")
    set(CMAKE_BUILD_POSTFIX "${CMAKE_MINSIZEREL_POSTFIX}")
elseif (CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    set(CMAKE_BUILD_POSTFIX "${CMAKE_RELWITHDEBINFO_POSTFIX}")
elseif (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(CMAKE_BUILD_POSTFIX "${CMAKE_DEBUG_POSTFIX}")
else ()
    set(CMAKE_BUILD_POSTFIX "")
endif ()

# set output directory
set(BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/bin" CACHE PATH "Directory for executable and shared libraries")
set(LIB_DIR "${CMAKE_CURRENT_BINARY_DIR}/lib" CACHE PATH "Directory for static libraries")

macro(nexengine_output_dirs target_name)
    set(runtime_dir "${BIN_DIR}/$<CONFIG>")
    set(library_dir "${BIN_DIR}/$<CONFIG>")
    set(archive_dir "${LIB_DIR}/$<CONFIG>")
    
    set_target_properties(${target_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${runtime_dir}"
            LIBRARY_OUTPUT_DIRECTORY "${library_dir}"
            ARCHIVE_OUTPUT_DIRECTORY "${archive_dir}"
    )
endmacro()

# add libraries
add_subdirectory(src)
add_subdirectory(assets)
add_subdirectory(examples)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()
