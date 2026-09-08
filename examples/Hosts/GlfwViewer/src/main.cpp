//
// Created by SherSpidey on 2026/5/22.
//

#include <NexEngine/App/Capability.h>
#include <NexEngine/Renderer/Device/Surface.h>

#include "NexApplication.h"

int main(int, char**)
{
    Nex::EngineCreateDesc create_desc;
    create_desc.scene_script = "assets/scripts/minimal.nex";
    create_desc.shader_search_path = "assets/shaders";
    create_desc.device_type = Nex::Render_Device_Type::D3D11;
    create_desc.description = Nex::ImmediateEngineDesc{};

    NexApplication application{create_desc};

    Nex::SurfaceDesc desc{};
    Nex::WindowSurface window{};
    
    auto& surface = desc.surface;
    auto& data = desc.data;
    
    surface.width = 1280;
    surface.height = 800;
    data = window;
    application.AddNexWindow(desc, 101);

    // surface.width = 600;
    // surface.height = 600;
    // window.title = "Nex Engine Second";
    // data = window;
    // application.AddNexWindow(desc, 101);

    while (!application.ShouldExit())
    {
        application.PoolEvents();

        // main loop here
        application.Tick();
    }

    return 0;
}
