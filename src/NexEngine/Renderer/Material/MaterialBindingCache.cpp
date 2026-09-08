//
// Created by SherSpidey on 2026/8/31.
//

#include <NexEngine/Renderer/Render/RenderBuffer.h>
#include <NexEngine/Renderer/Pipeline/PipelineRegistry.h>

#include <NexEngine/Utils/MaterialManager.h>
#include <NexEngine/Utils/ResourceManager.h>
#include <NexEngine/Utils/PersistentBufferManager.h>

#include <NexEngine/Renderer/Material/MaterialBindingCache.h>

using namespace Nex;
using namespace Diligent;

namespace
{
    PersistentTableID GetMaterialTable(MaterialType type, const MaterialManager& material_manager)
    {
        if (type == MaterialType::PBR) return material_manager.GetPBRTable();
        if (type == MaterialType::Phong) return material_manager.GetPhongTable();

        return InvalidPersistentTableID;
    }

    void BindTextureSlot(IShaderResourceBinding* binding, const char* name, const RefCntAutoPtr<ITextureView>& view)
    {
        if (!binding || !view) return;

        if (const auto resource_variable = binding->GetVariableByName(SHADER_TYPE_PIXEL, name))
        {
            resource_variable->Set(view.RawPtr());
        }
    }
}

IShaderResourceBinding* MaterialBindingCache::AcquireSRB(const DescriptorSRBKey& key,
                                                         IPipelineResourceSignature* signature,
                                                         PersistentBufferManager& buffer_manager,
                                                         const MaterialManager& material_manager,
                                                         ResourceManager& resource_manager)
{
    if (key.type == MaterialType::None) return nullptr;

    if (const auto it = cache_.find(key); it != cache_.end())
    {
        RefreshTableBinding(key.type, it->second, buffer_manager, material_manager);
        return it->second.binding.RawPtr();
    }

    if (!signature) return nullptr;

    const auto descriptor_set = resource_manager.GetResource<DescriptorSet>(key.handle);
    if (!descriptor_set) return nullptr;

    RefCntAutoPtr<IShaderResourceBinding> binding;
    signature->CreateShaderResourceBinding(&binding, true);
    if (!binding) return nullptr;

    BindTextures(key.type, *descriptor_set, binding.RawPtr());

    DescriptorSRBEntry entry;
    entry.binding = std::move(binding);

    if (const auto table = GetMaterialTable(key.type, material_manager);
        table != InvalidPersistentTableID)
    {
        if (const auto material_variable = entry.binding->GetVariableByName(
            SHADER_TYPE_PIXEL, ShaderSlots::Materials))
        {
            material_variable->Set(buffer_manager.View(table));
        }

        entry.material_version = buffer_manager.Version(table);
    }

    const auto [it, _] = cache_.emplace(key, std::move(entry));
    return it->second.binding.RawPtr();
}

void MaterialBindingCache::Clear() noexcept
{
    cache_.clear();
}

void MaterialBindingCache::BindTextures(MaterialType type,
                                        const DescriptorSet& descriptor_set,
                                        IShaderResourceBinding* binding) const
{
    if (type == MaterialType::PBR)
    {
        BindTextureSlot(binding, ShaderSlots::BaseColor, descriptor_set.base_color);
        BindTextureSlot(binding, ShaderSlots::Normal, descriptor_set.normal);
        BindTextureSlot(binding, ShaderSlots::ORM, descriptor_set.orm);
        BindTextureSlot(binding, ShaderSlots::Emissive, descriptor_set.emissive);
    }
    else if (type == MaterialType::Phong)
    {
        BindTextureSlot(binding, ShaderSlots::Diffuse, descriptor_set.diffuse);
        BindTextureSlot(binding, ShaderSlots::Specular, descriptor_set.specular);
        BindTextureSlot(binding, ShaderSlots::Normal, descriptor_set.normal);
    }
}

void MaterialBindingCache::RefreshTableBinding(MaterialType type,
                                               DescriptorSRBEntry& entry,
                                               PersistentBufferManager& buffer_manager,
                                               const MaterialManager& material_manager) const
{
    const auto table = GetMaterialTable(type, material_manager);
    if (table == InvalidPersistentTableID) return;

    const auto current_version = buffer_manager.Version(table);
    if (current_version == entry.material_version) return;

    if (const auto material_variable = entry.binding->GetVariableByName(
        SHADER_TYPE_PIXEL, ShaderSlots::Materials))
    {
        material_variable->Set(buffer_manager.View(table));
    }

    entry.material_version = current_version;
}
