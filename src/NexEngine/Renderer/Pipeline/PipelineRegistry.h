//
// Created by SherSpidey on 2026/8/18.
//

#ifndef NEXENGINE_PIPELINEREGISTRY_H
#define NEXENGINE_PIPELINEREGISTRY_H

#include <array>
#include <vector>
#include <cstdint>
#include <unordered_map>

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/PipelineState.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/PipelineResourceSignature.h>

#include <NexEngine/Core/Hash.h>
#include <NexEngine/Core/IDTypes.h>

#include <NexEngine/Renderer/Material/Material.h>

#include <NexEngine/Renderer/Renderer.h>

namespace Diligent
{
    struct IRenderDevice;
}

namespace Nex
{
    struct RenderCore;

    // how to render
    enum class RenderPassType : std::uint8_t
    {
        Forward = 0,
        ShadowMap,

        Count
    };

    struct TechniqueDesc
    {
        RenderPassType technique = RenderPassType::Forward;
        MaterialType material = MaterialType::PBR;

        RenderTopoFlag topology = RenderTopoFlag::Triangle;
    };

    struct ShaderKey
    {
        String file;
        String entry;
        Diligent::SHADER_TYPE type = Diligent::SHADER_TYPE_UNKNOWN;
    };

    struct PipelineKey
    {
        TechniqueID id = InvalidTechniqueID;
        Diligent::TEXTURE_FORMAT color_format = Diligent::TEX_FORMAT_UNKNOWN;
        Diligent::TEXTURE_FORMAT depth_format = Diligent::TEX_FORMAT_UNKNOWN;
    };

    struct PipelineLayout
    {
        Diligent::IPipelineResourceSignature* frame = nullptr;
        Diligent::IPipelineResourceSignature* material = nullptr;

        std::uint32_t signature_count = 0;
    };

    struct GraphicsPipeline
    {
        PipelineLayout layout;
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> state;
    };

    inline bool operator==(const TechniqueDesc& lhs, const TechniqueDesc& rhs) noexcept
    {
        return lhs.technique == rhs.technique && lhs.material == rhs.material && lhs.topology == rhs.topology;
    }

    inline bool operator==(const ShaderKey& lhs, const ShaderKey& rhs) noexcept
    {
        return lhs.file == rhs.file && lhs.entry == rhs.entry && lhs.type == rhs.type;
    }

    inline bool operator==(const PipelineKey& lhs, const PipelineKey& rhs) noexcept
    {
        return lhs.id == rhs.id && lhs.color_format == rhs.color_format && lhs.depth_format == rhs.depth_format;
    }
}

namespace std
{
    template <>
    struct hash<Nex::TechniqueDesc>
    {
        std::size_t operator()(const Nex::TechniqueDesc& tech) const noexcept
        {
            return Nex::ComputeHash(tech.technique, tech.material, tech.topology);
        }
    };

    template <>
    struct hash<Nex::ShaderKey>
    {
        std::size_t operator()(const Nex::ShaderKey& key) const noexcept
        {
            return Nex::ComputeHash(key.file, key.entry, key.type);
        }
    };

    template <>
    struct hash<Nex::PipelineKey>
    {
        std::size_t operator()(const Nex::PipelineKey& key) const noexcept
        {
            return Nex::ComputeHash(key.id, key.color_format, key.depth_format);
        }
    };
}

namespace Nex
{
    struct PipelineRegistry
    {
        explicit PipelineRegistry(Diligent::IRenderDevice*, Diligent::IShaderSourceInputStreamFactory*);

        PipelineRegistry(const PipelineRegistry&) = delete;
        PipelineRegistry& operator=(const PipelineRegistry&) = delete;
        PipelineRegistry(PipelineRegistry&&) noexcept = default;
        PipelineRegistry& operator=(PipelineRegistry&&) noexcept = default;

        TechniqueID Declare(const TechniqueDesc&);
        const GraphicsPipeline* Acquire(const PipelineKey&);

        Diligent::IPipelineResourceSignature* FrameSignature() const noexcept
        {
            return frame_signature_.RawPtr();
        }

    private:
        const GraphicsPipeline* Create(const TechniqueDesc&, const PipelineKey&);
        Diligent::IShader* Acquire(const char* file, const char* name, Diligent::SHADER_TYPE type);

        Diligent::IRenderDevice* device_;
        Diligent::IShaderSourceInputStreamFactory* shader_factory_;

        std::vector<TechniqueDesc> declared_;
        std::unordered_map<TechniqueDesc, TechniqueID> index_;
        std::unordered_map<PipelineKey, GraphicsPipeline> pipelines_;

        std::unordered_map<ShaderKey, Diligent::RefCntAutoPtr<Diligent::IShader>> shaders_cache_;

        Diligent::RefCntAutoPtr<Diligent::IPipelineResourceSignature> frame_signature_;
        std::array<Diligent::RefCntAutoPtr<Diligent::IPipelineResourceSignature>, raw(MaterialType::Count)>
        material_signatures_;
    };
}

#endif //NEXENGINE_PIPELINEREGISTRY_H
