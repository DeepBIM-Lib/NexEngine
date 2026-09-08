//
// Created by SherSpidey on 2026/7/6.
//

#include <algorithm>

#include <NexEngine/Core/MathUtils.h>
#include <NexEngine/Core/Transform.h>
#include <NexEngine/Core/TransformUtils.h>
#include <NexEngine/Core/Movement.h>

#include <NexEngine/Input/InputState.h>
#include <NexEngine/Input/Controller/Controller.h>
#include <NexEngine/Input/Controller/FlyController.h>

#include <NexEngine/Renderer/Camera/Camera.h>

#include <NexEngine/Input/Controller/FlyControllerSystem.h>

using namespace Nex;

namespace
{
    void FlyControllerUpdate(
        Rotor& rotor,
        Velocity& velocity,
        AngularVelocity& angular_velocity,
        FlyController& controller,
        const InputState& input_state,
        float delta_time
    )
    {
        if (delta_time <= 0.f) return;

        const auto acceleration_alpha =
            1.f - std::exp(-controller.acceleration_smooth * delta_time);
        const auto deceleration_alpha =
            1.f - std::exp(-controller.deceleration_smooth * delta_time);
        const auto angular_deceleration_alpha =
            1.f - std::exp(-controller.angular_deceleration_smooth * delta_time);

        const auto& keyboard = input_state.keyboard;
        const auto& mouse = input_state.mouse;
        const auto flythrough = IsButtonDown(mouse, MouseButton::Right);

        // update move speed
        const auto min_speed = std::max(controller.min_speed, 0.f);
        const auto max_speed = std::max(controller.max_speed, min_speed);

        if (flythrough && mouse.scroll_y != 0.f)
        {
            controller.move_speed = std::clamp(
                controller.move_speed * std::pow(controller.scroll_speed_scale, mouse.scroll_y),
                min_speed,
                max_speed
            );
        }
        else controller.move_speed = std::clamp(controller.move_speed, min_speed, max_speed);

        // mouse deltas are rotations for this frame
        // converting them to angular velocity keeps sensitivity stable when rotate integrates with delta_time
        if (flythrough)
        {
            const auto dx = mouse.x - mouse.px;
            const auto dy = mouse.y - mouse.py;

            angular_velocity.data = {
                .x = dy * controller.mouse_sensitivity / delta_time,
                .y = dx * controller.mouse_sensitivity / delta_time,
                .z = 0.f
            };
        }
        else
        {
            angular_velocity.data +=
                (Vector3{} - angular_velocity.data) * angular_deceleration_alpha;
        }

        // camera_pitch_limit is absolute, while rotor stores a delta from base
        const auto base_forward = normalize(ToForward(rotor.base));
        const auto base_pitch = -std::asin(std::clamp(base_forward.y, -1.f, 1.f));
        const auto pitch_limit = std::clamp(controller.camera_pitch_limit, 0.f, PI_DIV_2 - 0.001f);
        const auto min_pitch = -pitch_limit - base_pitch;
        const auto max_pitch = pitch_limit - base_pitch;

        rotor.data.x = std::clamp(rotor.data.x, min_pitch, max_pitch);
        angular_velocity.data.x = std::clamp(
            angular_velocity.data.x,
            (min_pitch - rotor.data.x) / delta_time,
            (max_pitch - rotor.data.x) / delta_time
        );

        auto next_rotor = rotor;
        next_rotor.data += angular_velocity.data * delta_time;
        const auto camera_rotation = ComposeRotorRotation(next_rotor);
        const auto forward = ToForward(camera_rotation);
        const auto right = ToRight(camera_rotation);

        Vector3 move_velocity{};
        if (flythrough && IsKeyDown(keyboard, Key::W))
        {
            move_velocity += forward;
        }
        if (flythrough && IsKeyDown(keyboard, Key::S))
        {
            move_velocity -= forward;
        }
        if (flythrough && IsKeyDown(keyboard, Key::A))
        {
            move_velocity -= right;
        }
        if (flythrough && IsKeyDown(keyboard, Key::D))
        {
            move_velocity += right;
        }
        if (flythrough && IsKeyDown(keyboard, Key::E))
        {
            move_velocity.y += 1.0f;
        }
        if (flythrough && IsKeyDown(keyboard, Key::Q))
        {
            move_velocity.y -= 1.0f;
        }

        // unity-style arrow navigation works without entering flythrough mode
        if (IsKeyDown(keyboard, Key::Up)) move_velocity += forward;
        if (IsKeyDown(keyboard, Key::Down)) move_velocity -= forward;
        if (IsKeyDown(keyboard, Key::Left)) move_velocity -= right;
        if (IsKeyDown(keyboard, Key::Right)) move_velocity += right;

        const auto sprint =
            IsKeyDown(keyboard, Key::LeftShift) || IsKeyDown(keyboard, Key::RightShift);
        const auto move_speed =
            sprint ? controller.move_speed * controller.sprint_scale : controller.move_speed;

        const auto move_velocity_len = length(move_velocity);
        if (move_velocity_len > 0.f)
        {
            const float velocity_scale = move_speed / move_velocity_len;
            move_velocity *= velocity_scale;
        }

        const auto move_alpha =
            move_velocity_len > 0.f ? acceleration_alpha : deceleration_alpha;

        velocity.data += (move_velocity - velocity.data) * move_alpha;
    }
}

void FlyControllerSystem::Register(flecs::world& world)
{
    world.component<FlyController>("FlyController")
         .member<float>("MinSpeed")
         .member<float>("MaxSpeed")
         .member<float>("MoveSpeed")
         .member<float>("ScrollSpeedScale")
         .member<float>("SprintScale")
         .member<float>("AccelerationSmooth")
         .member<float>("DecelerationSmooth")
         .member<float>("AngularMaxSpeed")
         .member<float>("AngularAccelerationSmooth")
         .member<float>("AngularDecelerationSmooth")
         .member<float>("MouseSensitivity")
         .member<float>("CameraPitchLimit")
         .add(flecs::With, world.component<Rotor>())
         .add(flecs::With, world.component<Controller>());

    world.system<
             Rotor,
             Velocity,
             AngularVelocity,
             FlyController,
             const InputState>("ControllerData")
         .kind(flecs::OnUpdate)
         .with<Perspective>()
         .each([](flecs::iter& it, std::size_t,
                  Rotor& rotor, Velocity& velocity,
                  AngularVelocity& angular_velocity, FlyController& controller,
                  const InputState& input_state)
         {
             auto delta_time = it.delta_time();

             FlyControllerUpdate(rotor, velocity, angular_velocity,
                                 controller, input_state, delta_time);
         });
}
