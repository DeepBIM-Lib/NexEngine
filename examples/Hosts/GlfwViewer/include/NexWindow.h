//
// Created by SherSpidey on 2026/5/29.
//

#ifndef NEXENGINE_NEXWINDOW_H
#define NEXENGINE_NEXWINDOW_H

#include <NexEngine/Core/Entity.h>
#include <NexEngine/Renderer/Device/Surface.h>

namespace Nex
{
    class Engine;
}

typedef struct GLFWwindow GLFWwindow;

class NexWindow {
public:
    explicit NexWindow(Nex::Engine* engine, Nex::SurfaceDesc& desc);
    ~NexWindow();
    
    bool ShouldClose() const;
    void SetWindowIcon(int icon) const;
    
protected:
    static void OnWindowResize(GLFWwindow* window, int width, int height);
    static void OnKeyEnvent(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void OnScrollEvent(GLFWwindow* window, double x, double y);
    static void OnCursorPosEvent(GLFWwindow* window, double x, double y);
    static void OnMouseButtonEvent(GLFWwindow* window, int button, int action, int mods);
    
    GLFWwindow* window_;
    Nex::Engine* engine_;
    Nex::Entity entity_;
};


#endif //NEXENGINE_NEXWINDOW_H
