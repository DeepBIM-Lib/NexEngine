//
// Created by SherSpidey on 2026/5/26.
//

#ifndef NEXENGINE_IMMEDIATEENGINE_H
#define NEXENGINE_IMMEDIATEENGINE_H

#include <NexEngine/App/EngineBase.h>

namespace Nex
{
    class ImmediateEngine : public EngineBase
    {
    public:
        ImmediateEngine();

        void Tick() override;
    };
}

#endif //NEXENGINE_IMMEDIATEENGINE_H
