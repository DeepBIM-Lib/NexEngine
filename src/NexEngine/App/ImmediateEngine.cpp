//
// Created by SherSpidey on 2026/5/26.
//

#include <NexEngine/App/ImmediateEngine.h>

using namespace Nex;

ImmediateEngine::ImmediateEngine()
{
}

void ImmediateEngine::Tick()
{
    EngineBase::Tick();
    
    world.progress();
    
    SubmitRetiredSignal();
    
    FinishFrame();
}
