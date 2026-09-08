//
// Created by SherSpidey on 2026/7/10.
//

#ifndef NEXENGINE_MESHMANAGER_H
#define NEXENGINE_MESHMANAGER_H

#include <array>
#include <string>
#include <vector>
#include <unordered_map>

#include <NexEngine/Geometry/Mesh.h>
#include <NexEngine/Geometry/MeshBuffer.h>
#include <NexEngine/Geometry/Primitives.h>

namespace Diligent
{
    struct IRenderDevice;
    struct IDeviceContext;
}

namespace Nex
{
    struct ResourceManager;

    struct MeshManager
    {
        MeshManager(Diligent::IRenderDevice*, Diligent::IDeviceContext*, ResourceManager&);

        MeshManager(const MeshManager&) = delete;
        MeshManager& operator=(const MeshManager&) = delete;
        MeshManager(MeshManager&&) noexcept = delete;
        MeshManager& operator=(MeshManager&&) noexcept = delete;

        MeshHandle Create(MeshDesc, ResourceManager&);
        MeshHandle CreatePrimitive(PrimitiveType, const void* data, ResourceManager&);
        bool Release(MeshHandle, ResourceManager&, std::uint64_t, bool&);

    private:
        // create page buffer
        bool CreatePage(std::uint32_t vertex_capacity, std::uint32_t index_capacity, bool dedicated = false);
        bool Allocate(std::uint32_t vertex_count, std::uint32_t index_count, Allocation& allocation);
        bool Upload(const MeshData&, Allocation&) const;

        void CreateAffinePrimitives(ResourceManager&);
        
        Diligent::IRenderDevice* device_;
        Diligent::IDeviceContext* context_;

        std::unordered_map<std::string, MeshHandle> key_cache_;
        std::unordered_map<std::uint64_t, std::vector<MeshHandle>> mesh_cache_;

        std::vector<BufferPage> pages_;
        std::array<MeshHandle, raw(PrimitiveType::Count)> primitive_meshes_;
    };
}

#endif //NEXENGINE_MESHMANAGER_H
