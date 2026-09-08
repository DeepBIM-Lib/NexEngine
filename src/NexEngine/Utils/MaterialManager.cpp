//
// Created by SherSpidey on 2026/8/7.
//

#include <filesystem>

#include <NexEngine/Renderer/Material/MaterialBuffer.h>

#include <NexEngine/Utils/DiligentUtils.h>
#include <NexEngine/Utils/InputDataUtils.h>
#include <NexEngine/Utils/ResourceManager.h>
#include <NexEngine/Utils/PersistentBufferManager.h>

#include <NexEngine/Utils/MaterialManager.h>

using namespace Nex;
using namespace Diligent;

namespace
{
    constexpr uint8_t White[] = {255, 255, 255, 255};
    constexpr uint8_t Black[] = {0, 0, 0, 255};
    constexpr uint8_t Normal[] = {128, 128, 255, 255};
    constexpr uint8_t Transparent[] = {0, 0, 0, 0};

    struct FallbackDesc
    {
        TextureFallback type;
        TEXTURE_FORMAT format;
        const void* data;
        const char* debug_name;
    };

    constexpr FallbackDesc texture_configs[]
    {
        {
            .type = TextureFallback::WhiteSRGB,
            .format = TEX_FORMAT_RGBA8_UNORM_SRGB,
            .data = White,
            .debug_name = "Fallback_White_sRGB"
        },
        {
            .type = TextureFallback::WhiteLinear,
            .format = TEX_FORMAT_RGBA8_UNORM,
            .data = White,
            .debug_name = "Fallback_White_Linear"
        },
        {
            .type = TextureFallback::Black,
            .format = TEX_FORMAT_RGBA8_UNORM,
            .data = Black,
            .debug_name = "Fallback_Black"
        },
        {
            .type = TextureFallback::FlatNormal,
            .format = TEX_FORMAT_RGBA8_UNORM,
            .data = Normal,
            .debug_name = "Fallback_Normal"
        },
        {
            .type = TextureFallback::Transparent,
            .format = TEX_FORMAT_RGBA8_UNORM,
            .data = Transparent,
            .debug_name = "Fallback_Transparent"
        },
    };

    void Create1x1Texture(IRenderDevice* device,
                          TEXTURE_FORMAT format,
                          const void* data,
                          const char* debug_name,
                          RefCntAutoPtr<ITextureView>& texture_view)
    {
        TextureDesc texture_desc;
        texture_desc.Name = debug_name;
        texture_desc.Type = RESOURCE_DIM_TEX_2D;
        texture_desc.Width = 1;
        texture_desc.Height = 1;
        texture_desc.MipLevels = 1;
        texture_desc.Format = format;
        texture_desc.BindFlags = BIND_SHADER_RESOURCE;
        texture_desc.Usage = USAGE_IMMUTABLE;

        TextureSubResData sub_res;
        sub_res.pData = data;
        sub_res.Stride = 4;

        TextureData texture_data;
        texture_data.pSubResources = &sub_res;
        texture_data.NumSubresources = 1;

        RefCntAutoPtr<ITexture> texture;
        device->CreateTexture(texture_desc, &texture_data, &texture);
        texture_view = texture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    }

    std::string MakeTextureKey(const Nex::String& path, TextureColorSpace color_space)
    {
        if (path.empty()) return {};

        return std::filesystem::absolute(
                std::filesystem::path{path.c_str()}).lexically_normal().generic_string() +
            (color_space == TextureColorSpace::SRGB ? "|srgb" : "|linear");
    }
}

MaterialManager::MaterialManager(IRenderDevice* device,
                                 PersistentBufferManager* buffer_manager,
                                 ResourceManager& resource_manager)
    : buffer_manager_{buffer_manager},
      device_{device}
{
    // create buffer table
    pbr_table_ = buffer_manager_->Declare(sizeof(PBRSlot), 256, "g_materials_pbr");
    phong_table_ = buffer_manager_->Declare(sizeof(PhongSlot), 256, "g_materials_phong");

    InitDefaultTextureViews();

    MaterialDesc pbr_material{};
    PBRMaterial pbr_data{};
    pbr_material.model = pbr_data;
    // using default pbr for fallback material
    fallback_material_ = CreateDefault(std::move(pbr_material), resource_manager);

    MaterialDesc phong_material{};
    PhongMaterial phong_data{};
    phong_material.model = phong_data;
    CreateDefault(std::move(phong_material), resource_manager);
}

MaterialHandle MaterialManager::Create(MaterialDesc desc, ResourceManager& resource_manager)
{
    if (auto validation = ValidateMaterialData(desc); !validation)
    {
        LOG_ERROR_MESSAGE("Create material validation failed: ", ToString(validation.code));

        return MakeInvalidHandle<MaterialResource>();
    }

    const auto hash = std::hash<MaterialDesc>{}(desc);

    // already there
    if (auto it = material_cache_.find(hash); it != material_cache_.end())
    {
        auto& bucket = it->second;
        for (const auto& handle : bucket)
        {
            const auto resource = resource_manager.GetResource(handle);
            if (resource && resource->data == desc)
            {
                if (!resource_manager.Retain(handle))
                {
                    LOG_ERROR_MESSAGE("Create material reference count overflow");
                    return MakeInvalidHandle<MaterialResource>();
                }

                return handle;
            }
        }
    }

    //create descriptor set
    MaterialTextures textures{};
    if (auto pbr = desc.model.get<PBRMaterial>())
    {
        textures.base_texture = pbr->base_texture;
        textures.normal_texture = pbr->normal_texture;
        textures.orm_texture = pbr->orm_texture;
        textures.emissive_texture = pbr->emissive_texture;
    }
    else if (auto phong = desc.model.get<PhongMaterial>())
    {
        textures.diffuse_texture = phong->diffuse_texture;
        textures.normal_texture = phong->normal_texture;
        textures.specular_texture = phong->specular_texture;
    }

    auto descriptor_handle = Create(std::move(textures), resource_manager);
    if (!IsHandleValid(descriptor_handle)) return MakeInvalidHandle<MaterialResource>();

    // create material
    MaterialResource material(std::move(desc), hash);

    // bind descriptor set
    material.descriptor = descriptor_handle;

    // material data registration
    if (material.data.model.get<PBRMaterial>())
    {
        auto data = MakePBRSlot(material.data);
        
        material.data_id = buffer_manager_->Acquire(pbr_table_);
        buffer_manager_->Write(pbr_table_, material.data_id, &data);
    }
    if (material.data.model.get<PhongMaterial>())
    {
        auto data = MakePhongSlot(material.data);
        
        material.data_id = buffer_manager_->Acquire(phong_table_);
        buffer_manager_->Write(phong_table_, material.data_id, &data);
    }

    // create handle
    auto material_handle = resource_manager.CreateResource<MaterialResource>(std::move(material));

    // add to cache
    material_cache_[hash].emplace_back(material_handle);

    return material_handle;
}

bool MaterialManager::Release(MaterialHandle handle,
                              ResourceManager& resource_manager,
                              std::uint64_t serial,
                              bool& retired)
{
    retired = false;

    auto material_resource = resource_manager.GetResource(handle);
    if (!material_resource) return false;

    // can not release a builtin resource
    if (material_resource->ref_count == 1 && material_resource->built_in) return false;

    if (material_resource->ref_count > 1)
    {
        --material_resource->ref_count;
        return true;
    }
    

    // time to release material resource
    if (!resource_manager.DestroyResource(handle, serial)) return false;
    
    // release material data
    if (material_resource->data.model.get<PBRMaterial>())
    {
        buffer_manager_->Release(pbr_table_, material_resource->data_id);
    }
    if (material_resource->data.model.get<PhongMaterial>())
    {
        buffer_manager_->Release(phong_table_, material_resource->data_id);
    }

    // clean cache
    if (auto it = material_cache_.find(material_resource->hash); it != material_cache_.end())
    {
        std::erase_if(it->second, [&](const MaterialHandle& material_handle)
        {
            return handle == material_handle;
        });

        if (it->second.empty()) material_cache_.erase(it);
    }

    // release descriptor now
    if (!Release(material_resource->descriptor, resource_manager, serial, retired)) return false;

    retired = true;
    return true;
}

void MaterialManager::InitDefaultTextureViews()
{
    for (const auto& config : texture_configs)
    {
        auto& texture_view = default_views_[raw(config.type)];
        Create1x1Texture(device_, config.format, config.data, config.debug_name, texture_view);
    }
}

MaterialHandle MaterialManager::CreateDefault(MaterialDesc desc, ResourceManager& resource_manager)
{
    auto handle = Create(std::move(desc), resource_manager);
    auto resource = resource_manager.GetResource(handle);

    if (!resource) return handle;

    resource->built_in = true;
    return handle;
}

DescriptorHandle MaterialManager::Create(MaterialTextures textures, ResourceManager& resource_manager)
{
    const auto hash = std::hash<MaterialTextures>{}(textures);

    // already there
    if (auto it = descriptor_set_cache_.find(hash); it != descriptor_set_cache_.end())
    {
        auto& bucket = it->second;
        for (const auto& handle : bucket)
        {
            const auto resource = resource_manager.GetResource(handle);
            if (resource && resource->data == textures)
            {
                if (!resource_manager.Retain(handle))
                {
                    LOG_ERROR_MESSAGE("Create descriptor set reference count overflow");
                    return MakeInvalidHandle<DescriptorSet>();
                }

                return handle;
            }
        }
    }

    // create descriptor set
    DescriptorSet descriptor_set(std::move(textures), hash);

    auto load_view = [this](const String& path,
                            RefCntAutoPtr<ITextureView>& view,
                            TextureColorSpace color_space,
                            const RefCntAutoPtr<ITextureView>& fallback_view)
    {
        if (!path.empty()) view = Load(path, color_space);
        if (!view) view = fallback_view;
    };

    auto& data = descriptor_set.data;
    // register texture view
    load_view(data.diffuse_texture, descriptor_set.diffuse, TextureColorSpace::SRGB,
              GetDefaultView(TextureFallback::WhiteSRGB));
    load_view(data.specular_texture, descriptor_set.specular, TextureColorSpace::Linear,
              GetDefaultView(TextureFallback::WhiteLinear));
    load_view(data.normal_texture, descriptor_set.normal, TextureColorSpace::Linear,
              GetDefaultView(TextureFallback::FlatNormal));
    load_view(data.base_texture, descriptor_set.base_color, TextureColorSpace::SRGB,
              GetDefaultView(TextureFallback::WhiteSRGB));
    load_view(data.orm_texture, descriptor_set.orm, TextureColorSpace::Linear,
              GetDefaultView(TextureFallback::WhiteLinear));
    load_view(data.emissive_texture, descriptor_set.emissive, TextureColorSpace::SRGB,
              GetDefaultView(TextureFallback::WhiteSRGB));

    auto descriptor_handle = resource_manager.CreateResource<DescriptorSet>(std::move(descriptor_set));

    // add to cache
    descriptor_set_cache_[hash].emplace_back(descriptor_handle);

    return descriptor_handle;
}

bool MaterialManager::Release(DescriptorHandle handle,
                              ResourceManager& resource_manager,
                              std::uint64_t serial,
                              bool& retired)
{
    retired = false;

    auto descriptor_set = resource_manager.GetResource(handle);
    if (!descriptor_set) return false;

    if (descriptor_set->ref_count > 1)
    {
        --descriptor_set->ref_count;
        return true;
    }

    // time to release descriptor set
    if (!resource_manager.DestroyResource(handle, serial)) return false;

    // clean cache
    if (auto it = descriptor_set_cache_.find(descriptor_set->hash); it != descriptor_set_cache_.end())
    {
        std::erase_if(
            it->second,
            [&](const DescriptorHandle& descriptor_handle)
            {
                return handle == descriptor_handle;
            });

        if (it->second.empty()) descriptor_set_cache_.erase(it);
    }

    retired = true;
    return true;
}

RefCntAutoPtr<ITextureView> MaterialManager::GetDefaultView(TextureFallback type) const
{
    return default_views_[raw(type)];
}

RefCntAutoPtr<ITextureView> MaterialManager::Load(const String& path, TextureColorSpace color_space)
{
    const auto key = MakeTextureKey(path, color_space);
    if (key.empty() || !device_) return {};

    if (const auto it = views_.find(key); it != views_.end()) return it->second;

    const auto texture = LoadTexture(
        device_, path.c_str(), color_space == TextureColorSpace::SRGB);
    if (!texture) return {};

    const auto view = texture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    if (!view) return {};

    auto [it, _] = views_.emplace(key, view);
    return it->second;
}
