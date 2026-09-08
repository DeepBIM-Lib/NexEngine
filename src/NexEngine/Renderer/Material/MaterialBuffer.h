//
// Created by SherSpidey on 2026/8/24.
//

#ifndef NEXENGINE_MATERIALBUFFER_H
#define NEXENGINE_MATERIALBUFFER_H

#include <NexEngine/Core/Math.h>

#include <NexEngine/Renderer/Material/Material.h>

namespace Nex
{
    struct PBRSlot
    {
        Vector4 base_color;
        Vector4 emissive;
        float metallic;
        float roughness;
        float normal_scale;
        float occlusion_strength;
        float alpha_mask;
        float alpha_cutoff;
        float padding[2];
    };
    
    struct PhongSlot
    {
        Vector4 diffuse;
        Vector4 specular;
        Vector4 emissive;
        float shininess;
        float alpha_mask;
        float alpha_cutoff;
        float padding;
    };
    
    static_assert(std::is_trivially_copyable_v<PBRSlot>);
    static_assert(std::is_trivially_copyable_v<PhongSlot>);
    static_assert(sizeof(PBRSlot) == 64);
    static_assert(sizeof(PhongSlot) == 64);

    inline PBRSlot MakePBRSlot(const MaterialDesc& material_desc) noexcept
    {
        PBRSlot slot{};
        
        if (auto pbr = material_desc.model.get<PBRMaterial>())
        {
            slot.base_color = pbr->base_color;
            slot.emissive = pbr->emissive;
            slot.metallic = pbr->metallic;
            slot.roughness = pbr->roughness;
            slot.normal_scale = pbr->normal_scale;
            slot.occlusion_strength = pbr->occlusion_strength;
            slot.alpha_mask = material_desc.material.alpha_mode == AlphaMode::Mask ? 1.0f : 0.0f;
            slot.alpha_cutoff = material_desc.material.alpha_cutoff;
        }
        
        return slot;
    }
    
    inline PhongSlot MakePhongSlot(const MaterialDesc& material_desc) noexcept
    {
        PhongSlot slot{};
        
        if (auto phong = material_desc.model.get<PhongMaterial>())
        {
            slot.diffuse = phong->diffuse;
            slot.specular = phong->specular;
            slot.emissive = phong->emissive;
            slot.shininess = phong->shininess;
            slot.alpha_mask = material_desc.material.alpha_mode == AlphaMode::Mask ? 1.0f : 0.0f;
            slot.alpha_cutoff = material_desc.material.alpha_cutoff;
        }
        
        return slot;
    }
}

#endif //NEXENGINE_MATERIALBUFFER_H
