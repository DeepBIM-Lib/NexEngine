//
// Created by SherSpidey on 2026/6/30.
//

#ifndef NEXENGINE_INPUTSTATE_H
#define NEXENGINE_INPUTSTATE_H

#include <NexEngine/Input/Input.h>

namespace Nex
{
    struct KeyState
    {
        bool keys_pre[raw(Key::Count)];
        bool keys_cur[raw(Key::Count)];
    };

    struct MouseState
    {
        bool buttons_pre[raw(MouseButton::Count)];
        bool buttons_cur[raw(MouseButton::Count)];

        float x;
        float y;
        float px;
        float py;

        float scroll_x;
        float scroll_y;
    };

    struct InputState
    {
        KeyState keyboard;
        MouseState mouse;
    };

    constexpr bool IsKeyDown(const KeyState& keyboard, Key key) noexcept
    {
        return keyboard.keys_cur[raw(key)];
    }

    constexpr bool JustKeyPressed(const KeyState& keyboard, Key key) noexcept
    {
        return keyboard.keys_cur[raw(key)] && !keyboard.keys_pre[raw(key)];
    }

    constexpr bool JustKeyReleased(const KeyState& keyboard, Key key) noexcept
    {
        return !keyboard.keys_cur[raw(key)] && keyboard.keys_pre[raw(key)];
    }

    constexpr bool IsButtonDown(const MouseState& mouse, MouseButton button) noexcept
    {
        return mouse.buttons_cur[raw(button)];
    }

    constexpr bool JustButtonPressed(const MouseState& mouse, MouseButton button) noexcept
    {
        return mouse.buttons_cur[raw(button)] && !mouse.buttons_pre[raw(button)];
    }

    constexpr bool JustButtonReleased(const MouseState& mouse, MouseButton button) noexcept
    {
        return !mouse.buttons_cur[raw(button)] && mouse.buttons_pre[raw(button)];
    }
}

#endif //NEXENGINE_INPUTSTATE_H
