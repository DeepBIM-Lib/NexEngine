//
// Created by SherSpidey on 2026/9/3.
//

#include <algorithm>

#include <NexEngine/Core/Transform.h>
#include <NexEngine/Renderer/Lighting/Light.h>
#include <NexEngine/Renderer/Lighting/LightingModule.h>

using namespace Nex;

namespace
{
    Vector3 ClampNonNegative(const Vector3& vec) noexcept
    {
        return Vector3{
            (std::max)(0.0f, vec.x),
            (std::max)(0.0f, vec.y),
            (std::max)(0.0f, vec.z)
        };
    }

    void SanitizeLightData(EnvironmentLight& light)
    {
        constexpr EnvironmentLight default_light{};

        if (!Finite(light.ambient_color)) light.ambient_color = default_light.ambient_color;
        if (!std::isfinite(light.ambient_intensity)) light.ambient_intensity = default_light.ambient_intensity;

        light.ambient_color = ClampNonNegative(light.ambient_color);
        light.ambient_intensity = (std::max)(0.0f, light.ambient_intensity);
    }

    void SanitizeLightData(DirectionalLight& light)
    {
        constexpr DirectionalLight default_light{};

        if (!Finite(light.direction)) light.direction = default_light.direction;
        if (!Finite(light.color)) light.color = default_light.color;
        if (!std::isfinite(light.intensity)) light.intensity = default_light.intensity;

        light.color = ClampNonNegative(light.color);
        light.intensity = (std::max)(0.0f, light.intensity);
    }

    void SanitizeLightData(PointLight& light)
    {
        constexpr PointLight default_light{};

        if (!Finite(light.color)) light.color = default_light.color;
        if (!std::isfinite(light.intensity)) light.intensity = default_light.intensity;
        if (!std::isfinite(light.range)) light.range = default_light.range;

        light.color = ClampNonNegative(light.color);
        light.intensity = (std::max)(0.0f, light.intensity);
        light.range = (std::max)(0.001f, light.range);
    }

    void SanitizeLightData(SpotLight& light)
    {
        constexpr SpotLight default_light{};

        if (!Finite(light.color)) light.color = default_light.color;
        if (!std::isfinite(light.intensity)) light.intensity = default_light.intensity;
        if (!std::isfinite(light.range)) light.range = default_light.range;
        if (!std::isfinite(light.inner_angle)) light.inner_angle = default_light.inner_angle;
        if (!std::isfinite(light.outer_angle)) light.outer_angle = default_light.outer_angle;

        light.color = ClampNonNegative(light.color);
        light.intensity = (std::max)(0.0f, light.intensity);
        light.range = (std::max)(0.001f, light.range);

        constexpr float max_half_angle = 1.5707963f;
        light.inner_angle = std::clamp(light.inner_angle, 0.0f, max_half_angle);
        light.outer_angle = std::clamp(light.outer_angle, 0.0f, max_half_angle);
        light.inner_angle = std::min(light.inner_angle, light.outer_angle);
    }

    void RegisterLightComponent(flecs::world& world)
    {
        world.component<EnvironmentLight>("EnvironmentLight")
             .on_set([](EnvironmentLight& light)
             {
                 SanitizeLightData(light);
             })
             .member<Vector3>("AmbientColor")
             .member<float>("AmbientIntensity");

        world.component<DirectionalLight>("DirectionalLight")
             .on_set([](DirectionalLight& light)
             {
                 SanitizeLightData(light);
             })
             .member<Vector3>("Direction")
             .member<Vector3>("Color")
             .member<float>("Intensity");

        world.component<PointLight>("PointLight")
             .on_set([](PointLight& light)
             {
                 SanitizeLightData(light);
             })
             .member<Vector3>("Color")
             .member<float>("Intensity")
             .member<float>("Range")
             .add(flecs::With, world.component<Position>());

        world.component<SpotLight>("SpotLight")
             .on_set([](SpotLight& light)
             {
                 SanitizeLightData(light);
             })
             .member<Vector3>("Color")
             .member<float>("Intensity")
             .member<float>("Range")
             .member<float>("InnerAngle")
             .member<float>("OuterAngle")
             .add(flecs::With, world.component<Rotation>());
    }
}

LightingModule::LightingModule(flecs::world& world)
{
    world.module<LightingModule>("NexEngine::Lighting");

    RegisterLightComponent(world);
}
