//
// Created by SherSpidey on 2026/5/29.
//

#include <iostream>

#define GLFW_EXPOSE_NATIVE_WIN32 
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <NexEngine/App/Engine.h>

#include "NexWindow.h"
#include "NexApplication.h"

NexApplication::NexApplication(const Nex::EngineCreateDesc& desc)
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        std::abort();
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    engine_ = Nex::CreateEngine(desc);
}

NexApplication::~NexApplication()
{
    Nex::DestroyEngine(engine_);
}

void NexApplication::AddNexWindow(Nex::SurfaceDesc& desc, int icon)
{
    windows_.emplace_back(std::make_unique<NexWindow>(engine_, desc));
    
    auto& window = windows_.back();
    window->SetWindowIcon(icon);
}

void NexApplication::PoolEvents()
{
    glfwPollEvents();
    
    std::erase_if(windows_, [](const auto& window)
    {
        return window->ShouldClose();
    });
}

void NexApplication::Tick() const
{
    engine_->Tick();
}

bool NexApplication::ShouldExit() const
{
    return windows_.empty();
}
