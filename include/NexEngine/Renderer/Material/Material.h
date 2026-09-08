//
// Created by SherSpidey on 2026/8/7.
//

#ifndef NEXENGINE_MATERIAL_H
#define NEXENGINE_MATERIAL_H

#include <NexEngine/Core/Math.h>
#include <NexEngine/Core/Handle.h>

namespace Nex
{
    enum class AlphaMode : std::uint8_t
    {
        Opaque = 0,
        Mask,
        Blend
    };
    
    enum class MaterialType : std::uint8_t
    {
        None = 0,
        PBR,
        Phong,

        Count
    };

    struct PBRMaterial
    {
        Vector4 base_color{1.0f, 1.0f, 1.0f, 1.0f};
        Vector4 emissive{0.0f, 0.0f, 0.0f, 1.0f};
        float metallic = 0.0f;
        float roughness = 0.5f;
        float normal_scale = 1.0f;
        float occlusion_strength = 1.0f;

        String base_texture;
        String normal_texture;
        String orm_texture;
        String emissive_texture;
    };

    struct PhongMaterial
    {
        Vector4 diffuse{1.0f, 1.0f, 1.0f, 1.0f};
        Vector4 specular{0.0f, 0.0f, 0.0f, 1.0f};
        Vector4 emissive{0.0f, 0.0f, 0.0f, 1.0f};
        float shininess = 32.0f;

        String diffuse_texture;
        String specular_texture;
        String normal_texture;
    };

    struct MaterialSource
    {
    };

    using DescriptorHandle = Handle<struct DescriptorSet>;
    using MaterialHandle = Handle<struct MaterialResource>;

    struct Material
    {
        AlphaMode alpha_mode = AlphaMode::Opaque;
        bool double_sided = false;

        float alpha_cutoff = 0.5;
    };

    struct MaterialDesc
    {
        Material material;

        Variant<
            PBRMaterial,
            PhongMaterial
        > model;
    };
}

#endif //NEXENGINE_MATERIAL_H
