//
// Created by SherSpidey on 2026/8/27.
//

#ifndef NEXENGINE_RENDERBUFFER_H
#define NEXENGINE_RENDERBUFFER_H

#include <NexEngine/Core/Math.h>
#include <NexEngine/Core/Transform.h>
#include <NexEngine/Renderer/Lighting/LightBuffer.h>

namespace Nex
{
    struct TransformSlot
    {
        Transform transform;
    };

    struct InstanceSlot
    {
        std::uint32_t transform_index;
        std::uint32_t material_index;
    };

    struct CameraConstants
    {
        Matrix4 projection;
        Matrix4 view;

        Vector3 position;
        float padding;
    };
    
    struct FrameMiscConstants
    {
        std::uint32_t local_light_count;
        std::uint32_t padding_0;
        std::uint32_t padding_1;
        std::uint32_t padding_2;
    };

    struct ForwardFrameConstants
    {
        CameraConstants camera;
        FrameMiscConstants frame;
    
        EnvironmentConstants environment;
        DirectionalLightConstants sun;
    };

    namespace ShaderSlots
    {
        inline constexpr auto Instances = "g_instances";
        inline constexpr auto Transforms = "g_transforms";
        inline constexpr auto Lights = "g_lights";
        inline constexpr auto FrameConstants = "g_frame_constants";
        inline constexpr auto BatchConstants = "g_batch_constants";

        inline constexpr auto Materials = "g_materials";

        inline constexpr auto BaseColor = "g_base_color_texture";
        inline constexpr auto Normal = "g_normal_texture";
        inline constexpr auto ORM = "g_orm_texture";
        inline constexpr auto Emissive = "g_emissive_texture";
        inline constexpr auto Diffuse = "g_diffuse_texture";
        inline constexpr auto Specular = "g_specular_texture";
    }
}

#endif //NEXENGINE_RENDERBUFFER_H
