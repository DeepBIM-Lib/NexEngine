set(Glfw_Host_ROOT ${Engine_Root}/examples/Hosts/GlfwViewer)

# add header files
file(GLOB HOST_HEADERS
        ${Glfw_Host_ROOT}/include/*.h
        ${Glfw_Host_ROOT}/*.h
)

# add source files explicitly
set(HOST_SOURCES
        src/main.cpp
        src/NexWindow.cpp
        src/NexApplication.cpp
)

# add glfw viewer
add_executable(GlfwViewer ${HOST_SOURCES} ${HOST_HEADERS})
nexengine_output_dirs(GlfwViewer)

add_dependencies(GlfwViewer AssetFiles)

target_link_libraries(GlfwViewer
        PRIVATE
        NexEngine::NexEngine
        glfw
)

target_include_directories(GlfwViewer
        PRIVATE
        $<BUILD_INTERFACE:${Glfw_Host_ROOT}/include>
)

# add resources
if (WIN32)
    set(SOURCE_FILES ${Engine_Root}/examples/Hosts/resources/resources.rc)
    target_sources(GlfwViewer PRIVATE ${SOURCE_FILES})
    # use icon
    target_compile_definitions(GlfwViewer PRIVATE IDI_ICON_TO_USE=IDI_ICON1)
endif ()