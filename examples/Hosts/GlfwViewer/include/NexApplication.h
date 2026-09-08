//
// Created by SherSpidey on 2026/5/29.
//

#ifndef NEXENGINE_NEXAPPLICATION_H
#define NEXENGINE_NEXAPPLICATION_H

#include <vector>
#include <memory>

class NexWindow;

namespace Nex
{
    class Engine;
    struct SurfaceDesc;
    struct EngineCreateDesc;
}

class NexApplication
{
public:
    explicit NexApplication(const Nex::EngineCreateDesc&);
    ~NexApplication();
    
    void AddNexWindow(Nex::SurfaceDesc& desc, int icon);
    
    void PoolEvents();
    void Tick() const;
    bool ShouldExit() const;
    
protected:
    Nex::Engine* engine_;
    std::vector<std::unique_ptr<NexWindow>> windows_;
};


#endif //NEXENGINE_NEXAPPLICATION_H
