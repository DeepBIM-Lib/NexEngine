//
// Created by SherSpidey on 2026/8/7.
//

#ifndef NEXENGINE_DESCRIPTORSET_H
#define NEXENGINE_DESCRIPTORSET_H

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/Texture.h>

#include <NexEngine/Core/Hash.h>

namespace Nex
{
    struct MaterialTextures
    {
        String diffuse_texture;
        String specular_texture;
        String normal_texture;
        String base_texture;
        String orm_texture;
        String emissive_texture;

        MaterialTextures() = default;

        explicit MaterialTextures(String diffuse, String specular, String normal)
            : diffuse_texture(std::move(diffuse)),
              specular_texture(std::move(specular)),
              normal_texture(std::move(normal))
        {
        }

        explicit MaterialTextures(String base, String normal, String orm, String emissive)
            : normal_texture(std::move(normal)),
              base_texture(std::move(base)),
              orm_texture(std::move(orm)),
              emissive_texture(std::move(emissive))
        {
        }
    };

    inline bool operator ==(const MaterialTextures& lhs, const MaterialTextures& rhs) noexcept
    {
        return lhs.diffuse_texture == rhs.diffuse_texture &&
            lhs.specular_texture == rhs.specular_texture &&
            lhs.normal_texture == rhs.normal_texture &&
            lhs.base_texture == rhs.base_texture &&
            lhs.orm_texture == rhs.orm_texture &&
            lhs.emissive_texture == rhs.emissive_texture;
    }
}

template <>
struct std::hash<Nex::MaterialTextures>
{
    std::size_t operator()(const Nex::MaterialTextures& textures) const noexcept
    {
        return Nex::ComputeHash(
            textures.diffuse_texture,
            textures.specular_texture,
            textures.normal_texture,
            textures.base_texture,
            textures.orm_texture,
            textures.emissive_texture
        );
    }
};

namespace Nex
{
    struct DescriptorSet
    {
        MaterialTextures data;

        Hash64 hash;
        std::uint32_t ref_count;

        Diligent::RefCntAutoPtr<Diligent::ITextureView> diffuse;
        Diligent::RefCntAutoPtr<Diligent::ITextureView> specular;
        Diligent::RefCntAutoPtr<Diligent::ITextureView> normal;
        Diligent::RefCntAutoPtr<Diligent::ITextureView> base_color;
        Diligent::RefCntAutoPtr<Diligent::ITextureView> orm;
        Diligent::RefCntAutoPtr<Diligent::ITextureView> emissive;

        explicit DescriptorSet(MaterialTextures desc, Hash64 desc_hash)
            : data{std::move(desc)},
              hash{desc_hash},
              ref_count{1}
        {
        }
        
        DescriptorSet(const DescriptorSet&) = delete;
        DescriptorSet& operator=(const DescriptorSet&) = delete;
        DescriptorSet(DescriptorSet&&) noexcept = default;
        DescriptorSet& operator=(DescriptorSet&&) noexcept = default;
    };

    inline bool operator ==(const DescriptorSet& lhs, const DescriptorSet& rhs) noexcept
    {
        return lhs.data == rhs.data;
    }
}

#endif //NEXENGINE_DESCRIPTORSET_H
