//
// Created by SherSpidey on 2026/8/31.
//

#ifndef NEXENGINE_MATERIALBINDINGCACHE_H
#define NEXENGINE_MATERIALBINDINGCACHE_H

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/ShaderResourceBinding.h>

#include <NexEngine/Core/IDTypes.h>
#include <NexEngine/Renderer/Material/Material.h>

namespace Nex
{
    struct PipelineRegistry;

    struct DescriptorSRBKey
    {
        MaterialType type;
        DescriptorHandle handle;
    };

    struct DescriptorSRBEntry
    {
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> binding;
        BufferVersion material_version = InvalidBufferVersion;
    };

    inline bool operator==(const DescriptorSRBKey& lhs, const DescriptorSRBKey& rhs)
    {
        return lhs.type == rhs.type && lhs.handle == rhs.handle;
    }
}

template <>
struct std::hash<Nex::DescriptorSRBKey>
{
    std::size_t operator()(const Nex::DescriptorSRBKey& key) const noexcept
    {
        return Nex::ComputeHash(key.type, Nex::HandleKey(key.handle));
    }
};

namespace Nex
{
    struct ResourceManager;
    struct MaterialManager;
    struct PersistentBufferManager;
    
    struct MaterialBindingCache
    {
        MaterialBindingCache() = default;

        MaterialBindingCache(const MaterialBindingCache&) = delete;
        MaterialBindingCache& operator=(const MaterialBindingCache&) = delete;
        MaterialBindingCache(MaterialBindingCache&&) = delete;
        MaterialBindingCache& operator=(MaterialBindingCache&&) = delete;

        Diligent::IShaderResourceBinding* AcquireSRB(const DescriptorSRBKey& key,
                                                     Diligent::IPipelineResourceSignature* signature,
                                                     PersistentBufferManager& buffer_manager,
                                                     const MaterialManager& material_manager,
                                                     ResourceManager& resource_manager);
        
        void Clear() noexcept;
        
    private:
        void BindTextures(MaterialType type,
                          const DescriptorSet& descriptor_set,
                          Diligent::IShaderResourceBinding* binding) const;

        void RefreshTableBinding(MaterialType type,
                                 DescriptorSRBEntry& entry,
                                 PersistentBufferManager& buffer_manager,
                                 const MaterialManager& material_manager) const;

        std::unordered_map<DescriptorSRBKey, DescriptorSRBEntry> cache_;
    };
}

#endif //NEXENGINE_MATERIALBINDINGCACHE_H
