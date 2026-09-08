//
// Created by SherSpidey on 2026/8/18.
//

#include <NexEngine/Utils/DiligentUtils.h>

#include <NexEngine/Renderer/Render/RenderBuffer.h>

#include <NexEngine/Renderer/Pipeline/PipelineRegistry.h>

using namespace Nex;
using namespace Diligent;

namespace
{
    constexpr LayoutElement StandardMeshVertexLayout[]
    {
        LayoutElement{0, 0, 3, VT_FLOAT32, False},
        LayoutElement{1, 1, 3, VT_FLOAT32, False},
        LayoutElement{2, 1, 2, VT_FLOAT32, False},
        LayoutElement{3, 1, 4, VT_FLOAT32, False}
    };

    struct TechniqueInfo
    {
        Nex::String name;
        Nex::String shader;
        const LayoutElement* input_layout;
        std::uint32_t input_layout_count;
    };

    // TODO: make this data-driven
    TechniqueInfo ResolveTechniqueInfo(RenderPassType technique, MaterialType material)
    {
        if (technique == RenderPassType::ShadowMap)
        {
            return {};
        }

        if (technique == RenderPassType::Forward)
        {
            if (material == MaterialType::PBR)
                return {
                    "Forward PBR Pass",
                    "forward_pbr",
                    StandardMeshVertexLayout,
                    _countof(StandardMeshVertexLayout)
                };

            if (material == MaterialType::Phong)
                return {
                    "Forward Phong Pass",
                    "forward_phong",
                    StandardMeshVertexLayout,
                    _countof(StandardMeshVertexLayout)
                };
        }

        return {};
    }

    const char* EntryPoint(SHADER_TYPE type)
    {
        if (type == SHADER_TYPE_VERTEX) return "VSMain";
        if (type == SHADER_TYPE_PIXEL) return "PSMain";

        // fail loud, not guess
        return nullptr;
    }

    void CreateFrameSignature(IRenderDevice* device, RefCntAutoPtr<IPipelineResourceSignature>& signature)
    {
        PipelineResourceDesc frame_resources[]
        {
            {
                SHADER_TYPE_VERTEX, ShaderSlots::Instances, 1, SHADER_RESOURCE_TYPE_BUFFER_SRV,
                SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC
            },
            {
                SHADER_TYPE_VERTEX, ShaderSlots::Transforms, 1, SHADER_RESOURCE_TYPE_BUFFER_SRV,
                SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC
            },
            {
                SHADER_TYPE_PIXEL, ShaderSlots::Lights, 1, SHADER_RESOURCE_TYPE_BUFFER_SRV,
                SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC
            },
            {
                SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, ShaderSlots::FrameConstants, 1,
                SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC
            },
            // uint + float3 padding keeps the D3D11/GL emulated-CB path at the 16-byte minimum
            {
                SHADER_TYPE_VERTEX, ShaderSlots::BatchConstants, 4, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
                SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC, PIPELINE_RESOURCE_FLAG_INLINE_CONSTANTS
            },
        };

        PipelineResourceSignatureDesc frame_desc;
        frame_desc.Name = "FrameSignature";
        frame_desc.BindingIndex = 0;
        frame_desc.Resources = frame_resources;
        frame_desc.NumResources = _countof(frame_resources);

        device->CreatePipelineResourceSignature(frame_desc, &signature);
    }

    void CreatePBRSignature(IRenderDevice* device, RefCntAutoPtr<IPipelineResourceSignature>& signature)
    {
        PipelineResourceDesc pbr_resources[]
        {
            {
                SHADER_TYPE_PIXEL, ShaderSlots::Materials, 1, SHADER_RESOURCE_TYPE_BUFFER_SRV,
                SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC
            },
            {
                SHADER_TYPE_PIXEL, ShaderSlots::BaseColor, 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV,
                SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE
            },
            {
                SHADER_TYPE_PIXEL, ShaderSlots::Normal, 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV,
                SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE
            },
            {
                SHADER_TYPE_PIXEL, ShaderSlots::ORM, 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV,
                SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE
            },
            {
                SHADER_TYPE_PIXEL, ShaderSlots::Emissive, 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV,
                SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE
            }
        };

        ImmutableSamplerDesc pbr_samplers[]
        {
            {SHADER_TYPE_PIXEL, ShaderSlots::BaseColor, {}},
            {SHADER_TYPE_PIXEL, ShaderSlots::Normal, {}},
            {SHADER_TYPE_PIXEL, ShaderSlots::ORM, {}},
            {SHADER_TYPE_PIXEL, ShaderSlots::Emissive, {}}
        };

        PipelineResourceSignatureDesc material_desc;
        material_desc.Name = "PBRSignature";
        material_desc.BindingIndex = 1;
        material_desc.UseCombinedTextureSamplers = True;
        material_desc.CombinedSamplerSuffix = "_sampler";
        material_desc.Resources = pbr_resources;
        material_desc.NumResources = _countof(pbr_resources);
        material_desc.ImmutableSamplers = pbr_samplers;
        material_desc.NumImmutableSamplers = _countof(pbr_samplers);

        device->CreatePipelineResourceSignature(material_desc, &signature);
    }

    void CreatePhongSignature(IRenderDevice* device, RefCntAutoPtr<IPipelineResourceSignature>& signature)
    {
        PipelineResourceDesc phong_resources[]
        {
            {
                SHADER_TYPE_PIXEL, ShaderSlots::Materials, 1, SHADER_RESOURCE_TYPE_BUFFER_SRV,
                SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC
            },
            {
                SHADER_TYPE_PIXEL, ShaderSlots::Diffuse, 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV,
                SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE
            },
            {
                SHADER_TYPE_PIXEL, ShaderSlots::Specular, 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV,
                SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE
            },
            {
                SHADER_TYPE_PIXEL, ShaderSlots::Normal, 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV,
                SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE
            }
        };

        ImmutableSamplerDesc phong_samplers[]
        {
            {SHADER_TYPE_PIXEL, ShaderSlots::Diffuse, {}},
            {SHADER_TYPE_PIXEL, ShaderSlots::Specular, {}},
            {SHADER_TYPE_PIXEL, ShaderSlots::Normal, {}}
        };

        PipelineResourceSignatureDesc material_desc;
        material_desc.Name = "PhongSignature";
        material_desc.BindingIndex = 1;
        material_desc.UseCombinedTextureSamplers = True;
        material_desc.CombinedSamplerSuffix = "_sampler";
        material_desc.Resources = phong_resources;
        material_desc.NumResources = _countof(phong_resources);
        material_desc.ImmutableSamplers = phong_samplers;
        material_desc.NumImmutableSamplers = _countof(phong_samplers);

        device->CreatePipelineResourceSignature(material_desc, &signature);
    }
}

PipelineRegistry::PipelineRegistry(IRenderDevice* device, IShaderSourceInputStreamFactory* shader_factory)
    : device_{device},
      shader_factory_{shader_factory}
{
    CreateFrameSignature(device_, frame_signature_);

    // create default pbr signature
    CreatePBRSignature(device_, material_signatures_[raw(MaterialType::PBR)]);

    // create default phong signature
    CreatePhongSignature(device_, material_signatures_[raw(MaterialType::Phong)]);
}

TechniqueID PipelineRegistry::Declare(const TechniqueDesc& desc)
{
    if (const auto it = index_.find(desc); it != index_.end())
        return it->second;

    declared_.emplace_back(desc);
    const auto id = static_cast<TechniqueID>(declared_.size());
    index_.emplace(desc, id);

    return id;
}

const GraphicsPipeline* PipelineRegistry::Acquire(const PipelineKey& key)
{
    if (key.id == InvalidTechniqueID || key.id > declared_.size()) return nullptr;

    if (const auto it = pipelines_.find(key); it != pipelines_.end())
        return &it->second;

    return Create(declared_[key.id - 1], key);
}

const GraphicsPipeline* PipelineRegistry::Create(const TechniqueDesc& desc, const PipelineKey& key)
{
    const auto info = ResolveTechniqueInfo(desc.technique, desc.material);

    const auto pso_name = info.name.c_str_suffix(" Pipeline");
    const auto vs_name = info.name.c_str_suffix(" Vertex Shader");
    const auto pixel_name = info.name.c_str_suffix(" Pixel Shader");
    const auto shader_file = info.shader.c_str_suffix(".hlsl");

    auto vertex_shader = Acquire(shader_file, vs_name, SHADER_TYPE_VERTEX);
    auto pixel_shader = Acquire(shader_file, pixel_name, SHADER_TYPE_PIXEL);
    if (!vertex_shader || !pixel_shader) return nullptr;

    IPipelineResourceSignature* signatures[2] = {};
    std::uint32_t signature_count = 0;

    signatures[signature_count++] = frame_signature_.RawPtr();

    IPipelineResourceSignature* material_signature = nullptr;
    if (desc.material != MaterialType::None)
    {
        material_signature = material_signatures_[raw(desc.material)].RawPtr();
        if (material_signature)
        {
            signatures[signature_count++] = material_signature;
        }
    }

    GraphicsPipelineStateCreateInfo pso_create_info;
    pso_create_info.PSODesc.Name = pso_name;
    pso_create_info.pVS = vertex_shader;
    pso_create_info.pPS = pixel_shader;
    pso_create_info.ppResourceSignatures = signatures;
    pso_create_info.ResourceSignaturesCount = signature_count;

    auto& graphics_pipeline = pso_create_info.GraphicsPipeline;

    graphics_pipeline.InputLayout.LayoutElements = info.input_layout;
    graphics_pipeline.InputLayout.NumElements = info.input_layout_count;

    graphics_pipeline.PrimitiveTopology = ToPrimitiveTopology(desc.topology);
    graphics_pipeline.NumRenderTargets = 1;
    graphics_pipeline.RTVFormats[0] = key.color_format;
    graphics_pipeline.DSVFormat = key.depth_format;

    GraphicsPipeline pipeline;
    device_->CreateGraphicsPipelineState(pso_create_info, &pipeline.state);
    if (!pipeline.state)
    {
        LOG_ERROR_MESSAGE("Failed to create PSO for technique ", static_cast<int>(desc.technique));
        return nullptr;
    }

    pipeline.layout.frame = frame_signature_.RawPtr();
    pipeline.layout.material = material_signature;
    pipeline.layout.signature_count = signature_count;

    const auto [it, _] = pipelines_.emplace(key, std::move(pipeline));
    return &it->second;
}

IShader* PipelineRegistry::Acquire(const char* file, const char* name, SHADER_TYPE type)
{
    auto entry = EntryPoint(type);
    if (!entry)
    {
        LOG_ERROR_MESSAGE("Unsupported shader stage for '", file, "'");
        return nullptr;
    }

    ShaderKey key{file, entry, type};
    if (const auto it = shaders_cache_.find(key); it != shaders_cache_.end())
        return it->second.RawPtr();

    ShaderCreateInfo shader_create_info;

    shader_create_info.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    shader_create_info.CompileFlags = SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;
    shader_create_info.pShaderSourceStreamFactory = shader_factory_;

    shader_create_info.Desc.Name = name;
    shader_create_info.Desc.ShaderType = type;
    shader_create_info.Desc.UseCombinedTextureSamplers = true;

    shader_create_info.EntryPoint = entry;
    shader_create_info.FilePath = file;

    RefCntAutoPtr<IShader> shader;
    device_->CreateShader(shader_create_info, &shader);

    const auto [it, _] = shaders_cache_.emplace(key, std::move(shader));
    return it->second.RawPtr();
}
