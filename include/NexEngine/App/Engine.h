//
// Created by SherSpidey on 2026/5/19.
//

#ifndef NEXENGINE_ENGINE_H
#define NEXENGINE_ENGINE_H

#include <NexEngine/Export.h>
#include <NexEngine/Core/Entity.h>
#include <NexEngine/App/Capability.h>
#include <NexEngine/Input/Input.h>
#include <NexEngine/Command/Command.h>
#include <NexEngine/Renderer/Device/Surface.h>

namespace Nex
{
    class Engine
    {
    public:
        virtual Entity CreateSurface(const SurfaceDesc&) = 0;
        virtual void CleanSurface(Entity) = 0;
        virtual void ResizeSurface(Entity, uint32_t width, uint32_t height) = 0;

        virtual void PushInput(const InputEvent&) = 0;

        virtual void SubmitCommand(const Command&) = 0;
        virtual void PollResponse(ResponseCallback, void*) = 0;

        virtual void Tick() = 0;

    protected:
        virtual ~Engine() = default;
    };

    NEXENGINE_API Engine* CreateEngine(const EngineCreateDesc&);
    NEXENGINE_API void DestroyEngine(Engine*);
}

#endif //NEXENGINE_ENGINE_H
