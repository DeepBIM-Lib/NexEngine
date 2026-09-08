//
// Created by SherSpidey on 2026/5/29.
//

#include <bit>

#define GLFW_EXPOSE_NATIVE_WIN32 
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <NexEngine/App/Engine.h>

#include "NexWindow.h"

using namespace Nex;

namespace
{
    constexpr Key MapKey(int glfw_key) noexcept
    {
        using Nex::Key;

        if (glfw_key >= GLFW_KEY_A && glfw_key <= GLFW_KEY_Z)
            return static_cast<Key>(raw(Key::A) + (glfw_key - GLFW_KEY_A));
        if (glfw_key >= GLFW_KEY_0 && glfw_key <= GLFW_KEY_9)
            return static_cast<Key>(raw(Key::Num0) + (glfw_key - GLFW_KEY_0));
        
        switch (glfw_key)
        {
        case GLFW_KEY_SPACE: return Key::Space;
        case GLFW_KEY_ENTER: return Key::Return;
        case GLFW_KEY_ESCAPE: return Key::Escape;
        case GLFW_KEY_BACKSPACE: return Key::BackSpace;
        case GLFW_KEY_TAB: return Key::Tab;
        case GLFW_KEY_DELETE: return Key::Delete;
        case GLFW_KEY_RIGHT: return Key::Right;
        case GLFW_KEY_LEFT: return Key::Left;
        case GLFW_KEY_DOWN: return Key::Down;
        case GLFW_KEY_UP: return Key::Up;
        case GLFW_KEY_LEFT_CONTROL: return Key::LeftCtrl;
        case GLFW_KEY_LEFT_ALT: return Key::LeftAlt;
        case GLFW_KEY_LEFT_SHIFT: return Key::LeftShift;
        case GLFW_KEY_RIGHT_CONTROL: return Key::RightCtrl;
        case GLFW_KEY_RIGHT_ALT: return Key::RightAlt;
        case GLFW_KEY_RIGHT_SHIFT: return Key::RightShift;
        case GLFW_KEY_F1: return Key::F1;
        case GLFW_KEY_F2: return Key::F2;
        case GLFW_KEY_F3: return Key::F3;
        case GLFW_KEY_F4: return Key::F4;
        case GLFW_KEY_F5: return Key::F5;
        case GLFW_KEY_F6: return Key::F6;
        case GLFW_KEY_F7: return Key::F7;
        case GLFW_KEY_F8: return Key::F8;
        case GLFW_KEY_F9: return Key::F9;
        case GLFW_KEY_F10: return Key::F10;
        case GLFW_KEY_F11: return Key::F11;
        case GLFW_KEY_F12: return Key::F12;
        case GLFW_KEY_CAPS_LOCK: return Key::Caps_Lock;
        default: return Key::Undefined;
        }
    }

    constexpr MouseButton MapMouseButton(int glfw_button) noexcept
    {
        using Nex::MouseButton;
        switch (glfw_button)
        {
        case GLFW_MOUSE_BUTTON_LEFT: return MouseButton::Left;
        case GLFW_MOUSE_BUTTON_RIGHT: return MouseButton::Right;
        case GLFW_MOUSE_BUTTON_MIDDLE: return MouseButton::Middle;
        case GLFW_MOUSE_BUTTON_4: return MouseButton::Extra1;
        case GLFW_MOUSE_BUTTON_5: return MouseButton::Extra2;
        default: return MouseButton::Undefined;
        }
    }
}

NexWindow::NexWindow(Engine* engine, SurfaceDesc& desc)
{
    engine_ = engine;

    auto window = desc.data.get<WindowSurface>();

    window_ = glfwCreateWindow(
        static_cast<int>(desc.surface.width),
        static_cast<int>(desc.surface.height),
        window->title.c_str(),
        nullptr,
        nullptr
    );

#ifdef _WIN32
    const HWND hwnd = glfwGetWin32Window(window_);
    window->w_id = std::bit_cast<std::uintptr_t>(hwnd);
#endif

    entity_ = engine_->CreateSurface(desc);

    // add this to window data
    glfwSetWindowUserPointer(window_, this);

    glfwSetWindowSizeCallback(window_, OnWindowResize);
    glfwSetKeyCallback(window_, OnKeyEnvent);
    glfwSetScrollCallback(window_, OnScrollEvent);
    glfwSetCursorPosCallback(window_, OnCursorPosEvent);
    glfwSetMouseButtonCallback(window_, OnMouseButtonEvent);
}

NexWindow::~NexWindow()
{
    engine_->CleanSurface(entity_);

    glfwDestroyWindow(window_);
    window_ = nullptr;
}

bool NexWindow::ShouldClose() const
{
    return glfwWindowShouldClose(window_);
}

void NexWindow::SetWindowIcon(int icon) const
{
#ifdef _WIN32
    const HINSTANCE h_instance = GetModuleHandle(nullptr);

    HICON h_icon = LoadIcon(h_instance, MAKEINTRESOURCE(icon));
    if (!h_icon) return;

    const HWND hwnd = glfwGetWin32Window(window_);

    SendMessage(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(h_icon));
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(h_icon));
#endif
}

void NexWindow::OnWindowResize(GLFWwindow* window, int width, int height)
{
    auto instance = static_cast<NexWindow*>(glfwGetWindowUserPointer(window));

    instance->engine_->ResizeSurface(instance->entity_, width, height);
}

void NexWindow::OnKeyEnvent(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (action == GLFW_REPEAT) return;

    auto instance = static_cast<NexWindow*>(glfwGetWindowUserPointer(window));
    const auto k = MapKey(key);
    if (k == Key::Undefined) return;

    InputEvent event;
    if (action == GLFW_PRESS)
        event.data.emplace<KeyPress>(KeyPress{k});
    else
        event.data.emplace<KeyRelease>(KeyRelease{k});

    instance->engine_->PushInput(event);
}

void NexWindow::OnScrollEvent(GLFWwindow* window, double x, double y)
{
    auto instance = static_cast<NexWindow*>(glfwGetWindowUserPointer(window));

    InputEvent event;
    event.data.emplace<MouseScroll>(MouseScroll{
        static_cast<float>(x), static_cast<float>(y)
    });
    instance->engine_->PushInput(event);
}

void NexWindow::OnCursorPosEvent(GLFWwindow* window, double x, double y)
{
    auto instance = static_cast<NexWindow*>(glfwGetWindowUserPointer(window));

    InputEvent event;
    event.data.emplace<MouseMove>(MouseMove{
        static_cast<float>(x), static_cast<float>(y)
    });
    instance->engine_->PushInput(event);
}

void NexWindow::OnMouseButtonEvent(GLFWwindow* window, int button, int action, int /*mods*/)
{
    auto instance = static_cast<NexWindow*>(glfwGetWindowUserPointer(window));
    const auto b = MapMouseButton(button);
    if (b == MouseButton::Undefined) return;

    InputEvent event;
    if (action == GLFW_PRESS)
        event.data.emplace<MouseButtonPress>(MouseButtonPress{b});
    else
        event.data.emplace<MouseButtonRelease>(MouseButtonRelease{b});

    instance->engine_->PushInput(event);
}
