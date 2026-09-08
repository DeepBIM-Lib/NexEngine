//
// Created by SherSpidey on 2026/8/7.
//

#ifndef NEXENGINE_MATERIALMANAGER_H
#define NEXENGINE_MATERIALMANAGER_H

#include <cstdint>
#include <array>
#include <vector>
#include <unordered_map>

#include <NexEngine/Renderer/Material/Texture.h>
#include <NexEngine/Renderer/Material/Material.h>
#include <NexEngine/Renderer/Material/MaterialResource.h>

namespace Diligent
{
    struct IRenderDevice;
    struct ITextureView;
}

namespace Nex
{
    struct ResourceManager;
    struct PersistentBufferManager;

    struct MaterialManager
    {
        MaterialManager(Diligent::IRenderDevice*, PersistentBufferManager*, ResourceManager&);

        MaterialManager(const MaterialManager&) = delete;
        MaterialManager& operator=(const MaterialManager&) = delete;
        MaterialManager(MaterialManager&&) noexcept = delete;
        MaterialManager& operator=(MaterialManager&&) noexcept = delete;

        MaterialHandle Create(MaterialDesc, ResourceManager&);
        bool Release(MaterialHandle, ResourceManager&, std::uint64_t, bool&);

        MaterialHandle GetFallback() const { return fallback_material_; }
        PersistentTableID GetPBRTable() const { return pbr_table_; }
        PersistentTableID GetPhongTable() const { return phong_table_; }

    private:
        // create default fallback views
        void InitDefaultTextureViews();

        MaterialHandle CreateDefault(MaterialDesc, ResourceManager&);
        DescriptorHandle Create(MaterialTextures, ResourceManager&);
        bool Release(DescriptorHandle, ResourceManager&, std::uint64_t, bool&);

        Diligent::RefCntAutoPtr<Diligent::ITextureView> GetDefaultView(TextureFallback type) const;
        Diligent::RefCntAutoPtr<Diligent::ITextureView> Load(const String&, TextureColorSpace);

        MaterialHandle fallback_material_;

        std::unordered_map<std::uint64_t, std::vector<MaterialHandle>> material_cache_;
        std::unordered_map<std::uint64_t, std::vector<DescriptorHandle>> descriptor_set_cache_;

        PersistentTableID pbr_table_;
        PersistentTableID phong_table_;
        PersistentBufferManager* buffer_manager_ = nullptr;

        std::unordered_map<std::string, Diligent::RefCntAutoPtr<Diligent::ITextureView>> views_;
        std::array<Diligent::RefCntAutoPtr<Diligent::ITextureView>, raw(TextureFallback::Count)> default_views_;
        Diligent::IRenderDevice* device_ = nullptr;
    };
}
#endif //NEXENGINE_MATERIALMANAGER_H
