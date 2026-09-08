//
// Created by SherSpidey on 2026/5/26.
//

#ifndef NEXENGINE_ENGINEBASE_H
#define NEXENGINE_ENGINEBASE_H

#include <flecs.h>
#include <NexEngine/App/Engine.h>

namespace Nex
{
    struct EngineBase : Engine
    {
        virtual void ImportECSModules();
        virtual void CreateDeviceAndResource(Render_Device_Type type, const String& shader_path = {});
        virtual void InitECSWorld(const String& script = {}) const;
        virtual void Destroy();

        Entity CreateSurface(const SurfaceDesc&) override;
        void CleanSurface(Entity) override;
        void ResizeSurface(Entity, std::uint32_t width, std::uint32_t height) override;

        void PushInput(const InputEvent&) override;

        void SubmitCommand(const Command&) override;
        void PollResponse(ResponseCallback, void*) override;

        void Tick() override;
        void SubmitRetiredSignal() const;
        void FinishFrame() const;

        flecs::world world;
    };
}

#endif //NEXENGINE_ENGINEBASE_H
