//
// Created by SherSpidey on 2026/8/7.
//

#ifndef NEXENGINE_MATERIALRESOURCE_H
#define NEXENGINE_MATERIALRESOURCE_H

#include <NexEngine/Core/Hash.h>
#include <NexEngine/Core/IDTypes.h>

#include <NexEngine/Renderer/Material/Material.h>
#include <NexEngine/Renderer/Material/DescriptorSet.h>

namespace Nex
{
    inline bool operator ==(const PBRMaterial& lhs, const PBRMaterial& rhs) noexcept
    {
        return lhs.base_color == rhs.base_color &&
            lhs.emissive == rhs.emissive &&
            lhs.base_texture == rhs.base_texture &&
            lhs.normal_texture == rhs.normal_texture &&
            lhs.orm_texture == rhs.orm_texture &&
            lhs.emissive_texture == rhs.emissive_texture &&
            lhs.metallic == rhs.metallic &&
            lhs.roughness == rhs.roughness &&
            lhs.normal_scale == rhs.normal_scale &&
            lhs.occlusion_strength == rhs.occlusion_strength;
    }

    inline bool operator ==(const PhongMaterial& lhs, const PhongMaterial& rhs) noexcept
    {
        return lhs.diffuse == rhs.diffuse &&
            lhs.specular == rhs.specular &&
            lhs.emissive == rhs.emissive &&
            lhs.diffuse_texture == rhs.diffuse_texture &&
            lhs.specular_texture == rhs.specular_texture &&
            lhs.normal_texture == rhs.normal_texture &&
            lhs.shininess == rhs.shininess;
    }

    inline bool operator ==(const Material& lhs, const Material& rhs) noexcept
    {
        return lhs.alpha_mode == rhs.alpha_mode &&
            lhs.double_sided == rhs.double_sided &&
            lhs.alpha_cutoff == rhs.alpha_cutoff;
    }

    inline bool operator ==(const MaterialDesc& lhs, const MaterialDesc& rhs) noexcept
    {
        if (lhs.material != rhs.material) return false;

        if (const auto lhs_pbr = lhs.model.get<PBRMaterial>())
        {
            const auto rhs_pbr = rhs.model.get<PBRMaterial>();
            return rhs_pbr && *lhs_pbr == *rhs_pbr;
        }

        const auto lhs_phong = lhs.model.get<PhongMaterial>();
        const auto rhs_phong = rhs.model.get<PhongMaterial>();

        return lhs_phong && rhs_phong && *lhs_phong == *rhs_phong;
    }
}

namespace std
{
    template <>
    struct hash<Nex::PBRMaterial>
    {
        std::size_t operator()(const Nex::PBRMaterial& data) const noexcept
        {
            return Nex::ComputeHash(
                data.base_color,
                data.emissive,
                data.metallic,
                data.roughness,
                data.normal_scale,
                data.occlusion_strength,
                data.base_texture,
                data.normal_texture,
                data.orm_texture,
                data.emissive_texture
            );
        }
    };

    template <>
    struct hash<Nex::PhongMaterial>
    {
        std::size_t operator()(const Nex::PhongMaterial& data) const noexcept
        {
            return Nex::ComputeHash(
                data.diffuse,
                data.specular,
                data.emissive,
                data.shininess,
                data.diffuse_texture,
                data.specular_texture,
                data.normal_texture
            );
        }
    };

    template <>
    struct hash<Nex::MaterialDesc>
    {
        std::size_t operator()(const Nex::MaterialDesc& data) const noexcept
        {
            Nex::StableHash64Builder hash;

            hash.Append(data.material.alpha_mode);
            hash.Append(data.material.double_sided);
            hash.Append(data.material.alpha_cutoff);

            hash.Append(data.model.index());
            if (const auto pbr = data.model.get<Nex::PBRMaterial>())
            {
                Nex::HashAppend(hash, pbr->base_color);
                Nex::HashAppend(hash, pbr->emissive);

                hash.Append(pbr->metallic);
                hash.Append(pbr->roughness);
                hash.Append(pbr->normal_scale);
                hash.Append(pbr->occlusion_strength);

                hash.AppendString(pbr->base_texture);
                hash.AppendString(pbr->normal_texture);
                hash.AppendString(pbr->orm_texture);
                hash.AppendString(pbr->emissive_texture);
            }
            else if (const auto phong = data.model.get<Nex::PhongMaterial>())
            {
                Nex::HashAppend(hash, phong->diffuse);
                Nex::HashAppend(hash, phong->specular);
                Nex::HashAppend(hash, phong->emissive);

                hash.Append(phong->shininess);

                hash.AppendString(phong->diffuse_texture);
                hash.AppendString(phong->specular_texture);
                hash.AppendString(phong->normal_texture);
            }

            return hash.Finish();
        }
    };
}

namespace Nex
{
    struct MaterialResource
    {
        MaterialDesc data;

        Hash64 hash;
        std::uint32_t ref_count;
        bool built_in;

        PersistentSlotIndex data_id;
        DescriptorHandle descriptor;

        explicit MaterialResource(MaterialDesc desc, Hash64 desc_hash)
            : data{std::move(desc)},
              hash{desc_hash},
              ref_count{1},
              built_in{false},
              data_id{InvalidPersistentSlotIndex},
              descriptor{DescriptorHandle{}}
        {
        }

        MaterialResource(const MaterialResource&) = delete;
        MaterialResource& operator=(const MaterialResource&) = delete;
        MaterialResource(MaterialResource&&) noexcept = default;
        MaterialResource& operator=(MaterialResource&&) noexcept = default;
    };
    
    inline bool operator ==(const MaterialResource& lhs, const MaterialResource& rhs) noexcept
    {
        return lhs.data == rhs.data;
    }
}

#endif //NEXENGINE_MATERIALRESOURCE_H
