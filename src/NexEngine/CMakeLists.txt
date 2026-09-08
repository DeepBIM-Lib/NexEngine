option(NexEngine_BUILD_SHARED "Build NexEngine as a shared library" OFF)

# add header files
file(GLOB HEADERS
        ${Engine_Root}/include/NexEngine/*.h
        ${Engine_Root}/include/NexEngine/*/*.h
        ${Engine_Root}/include/NexEngine/*/*/*.h

        ${Engine_Root}/src/NexEngine/*.h
        ${Engine_Root}/src/NexEngine/*/*.h
        ${Engine_Root}/src/NexEngine/*/*/*.h
)

# add source files explicitly
set(SOURCES
        Core/CoreModule.cpp

        Input/InputModule.cpp
        Input/Controller/FlyControllerSystem.cpp

        Geometry/GeometryModule.cpp
        Geometry/Primitives/PrimitiveShape.cpp
        Geometry/Primitives/SpherifiedCube.cpp
        Geometry/Primitives/Revolutions.cpp
        Geometry/Primitives/Grids.cpp
        Geometry/Primitives/IcoSphere.cpp

        Renderer/RendererModule.cpp

        Renderer/Camera/CameraModule.cpp

        Renderer/Device/SurfaceResource.cpp
        Renderer/Device/DeviceModule.cpp

        Renderer/Pipeline/FramePlan.cpp
        Renderer/Pipeline/PipelineRegistry.cpp

        Renderer/Lighting/LightingModule.cpp

        Renderer/Material/MaterialModule.cpp
        Renderer/Material/MaterialBindingCache.cpp

        Renderer/Render/RenderCore.cpp
        Renderer/Render/RenderRuntime.cpp
        Renderer/Render/RenderFrame.cpp
        Renderer/Render/RenderPipelineModule.cpp

        Utils/DiligentUtils.cpp
        Utils/InputDataUtils.cpp
        Utils/MeshManager.cpp
        Utils/MaterialManager.cpp
        Utils/PersistentBufferManager.cpp

        App/Engine.cpp
        App/EngineModule.cpp
        App/EngineBase.cpp
        App/ImmediateEngine.cpp
        App/DeferredEngine.cpp
)

if (NexEngine_BUILD_SHARED)
    add_library(NexEngine SHARED ${HEADERS} ${SOURCES})
    target_compile_definitions(
            NexEngine
            PUBLIC NexEngine_BUILD_SHARED
            PRIVATE NexEngine_EXPORTS
    )
else ()
    add_library(NexEngine STATIC ${HEADERS} ${SOURCES})
endif ()
nexengine_output_dirs(NexEngine)

add_library(NexEngine::NexEngine ALIAS NexEngine)

target_include_directories(NexEngine
        PUBLIC
        $<BUILD_INTERFACE:${Engine_Root}/include>
        $<BUILD_INTERFACE:${Engine_Root}/include/NexEngine/externals>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${Engine_Root}/externals
        ${Engine_Root}/src
)

target_link_libraries(NexEngine
        PRIVATE
        Diligent-GraphicsTools
        Diligent-TextureLoader
        flecs::flecs_static
)

if (D3D11_SUPPORTED)
    target_link_libraries(NexEngine PRIVATE Diligent-GraphicsEngineD3D11-static)
endif ()
if (D3D12_SUPPORTED)
    target_link_libraries(NexEngine PRIVATE Diligent-GraphicsEngineD3D12-static)
endif ()
#if(GL_SUPPORTED)
#    target_link_libraries(NexEngine PRIVATE Diligent-GraphicsEngineOpenGL-static)
#endif()
if (VULKAN_SUPPORTED)
    target_link_libraries(NexEngine PRIVATE Diligent-GraphicsEngineVk-static)
endif ()
