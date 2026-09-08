//
// Created by SherSpidey on 2026/6/30.
//

#include <NexEngine/Input/InputState.h>
#include <NexEngine/Input/InputModule.h>

using namespace Nex;

InputModule::InputModule(flecs::world& world)
{
    world.module<InputModule>("NexEngine::Input");

    world.component<KeyState>()
         .member<bool>("Pre", raw(Key::Count))
         .member<bool>("Cur", raw(Key::Count));

    world.component<MouseState>()
         .member<bool>("Pre", raw(MouseButton::Count))
         .member<bool>("Cur", raw(MouseButton::Count))
         .member<float>("X")
         .member<float>("Y")
         .member<float>("PX")
         .member<float>("PY")
         .member<float>("ScrollX")
         .member<float>("ScrollY");

    world.component<InputState>()
         .member<KeyState>("Keyboard")
         .member<MouseState>("Mouse")
         .add(flecs::Singleton);

    world.set<InputState>({});

    world.component<InputBuffer>()
         .add(flecs::Singleton);

    world.set<InputBuffer>({});

    world.system<InputState>("InputSystem")
         .kind(flecs::PreUpdate)
         .each([](flecs::iter& it, size_t, InputState& state)
         {
             // pass input state
             std::memcpy(state.keyboard.keys_pre, state.keyboard.keys_cur, raw(Key::Count));
             std::memcpy(state.mouse.buttons_pre, state.mouse.buttons_cur, raw(MouseButton::Count));

             state.mouse.px = state.mouse.x;
             state.mouse.py = state.mouse.y;

             state.mouse.scroll_x = 0;
             state.mouse.scroll_y = 0;

             // consume input buffer
             auto world = it.world();
             auto& input_buffer = world.get_mut<InputBuffer>();

             if (!input_buffer.events.empty())
             {
                 for (auto& event : input_buffer.events)
                 {
                     if (auto key_press = event.data.get<KeyPress>())
                     {
                         state.keyboard.keys_cur[raw(key_press->key)] = true;
                     }
                     else if (auto key_release = event.data.get<KeyRelease>())
                     {
                         state.keyboard.keys_cur[raw(key_release->key)] = false;
                     }
                     else if (auto button_press = event.data.get<MouseButtonPress>())
                     {
                         state.mouse.buttons_cur[raw(button_press->button)] = true;
                     }
                     else if (auto button_release = event.data.get<MouseButtonRelease>())
                     {
                         state.mouse.buttons_cur[raw(button_release->button)] = false;
                     }
                     else if (auto mouse_scroll = event.data.get<MouseScroll>())
                     {
                         state.mouse.scroll_x = mouse_scroll->dx;
                         state.mouse.scroll_y = mouse_scroll->dy;
                     }
                     else if (auto mouse_move = event.data.get<MouseMove>())
                     {
                         state.mouse.x = mouse_move->x;
                         state.mouse.y = mouse_move->y;
                     }
                 }
                 input_buffer.events.clear();
             }
         });
}
