//
// Created by SherSpidey on 2026/6/30.
//

#ifndef NEXENGINE_INPUT_H
#define NEXENGINE_INPUT_H

#include <NexEngine/Core/BaseType.h>

namespace Nex
{
    enum class Key : std::uint16_t
    {
        Undefined = 0,
        Any,

        Return,
        Escape,
        BackSpace,
        Tab,
        Space,
        Delete,
        Caps_Lock,

        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

        A, B, C, D, E, F, G, H, I, J, K, L,
        M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

        Right,
        Left,
        Down,
        Up,

        LeftCtrl,
        LeftAlt,
        LeftShift,
        RightCtrl,
        RightAlt,
        RightShift,

        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,

        Count
    };

    enum class MouseButton : std::uint8_t
    {
        Undefined = 0,
        Any,

        Left,
        Right,
        Middle,

        Extra1,
        Extra2,

        Count,
    };

    struct KeyPress
    {
        Key key;
    };

    struct KeyRelease
    {
        Key key;
    };

    struct MouseButtonPress
    {
        MouseButton button;
    };

    struct MouseButtonRelease
    {
        MouseButton button;
    };

    struct MouseScroll
    {
        float dx;
        float dy;
    };

    struct MouseMove
    {
        float x;
        float y;
    };

    struct InputEvent
    {
        Variant<
            KeyPress,
            KeyRelease,
            MouseButtonPress,
            MouseButtonRelease,
            MouseScroll,
            MouseMove
        > data;
    };

    struct InputBuffer
    {
        Array<InputEvent> events;
    };
}

#endif //NEXENGINE_INPUT_H
