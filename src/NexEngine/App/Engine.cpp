//
// Created by SherSpidey on 2026/5/19.
//

#include <NexEngine/App/Engine.h>

#include <NexEngine/App/ImmediateEngine.h>
#include <NexEngine/App/DeferredEngine.h>

using namespace Nex;

Engine* Nex::CreateEngine(const EngineCreateDesc& create_desc)
{
    EngineBase* engine;

    if (auto defer_desc = create_desc.description.get<DeferredEngineDesc>())
    {
        //TODO: do deferred-engine init stuff here
        engine = new DeferredEngine;
    }
    else
    {
        auto imme_desc = create_desc.description.get<ImmediateEngineDesc>();
        //TODO: do immediate-engine init stuff here
        engine = new ImmediateEngine;
    }

    // import ecs modules
    engine->ImportECSModules();

    // create render device and resources
    engine->CreateDeviceAndResource(create_desc.device_type, create_desc.shader_search_path);

    // start ecs world
    engine->InitECSWorld(create_desc.scene_script);

    return engine;
}

void Nex::DestroyEngine(Engine* engine)
{
    auto engine_base = static_cast<EngineBase*>(engine);
    engine_base->Destroy();
}
