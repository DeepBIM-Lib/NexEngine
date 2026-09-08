//
// Created by SherSpidey on 2026/8/21.
//

#ifndef NEXENGINE_PERSISTENTBUFFERMANAGER_H
#define NEXENGINE_PERSISTENTBUFFERMANAGER_H

#include <cstdint>

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsTools/interface/DynamicBuffer.hpp>
#include <DiligentCore/Graphics/GraphicsTools/interface/GPUUploadManager.h>
#include <DiligentCore/Graphics/GraphicsTools/interface/GPUCompletionAwaitQueue.hpp>

#include <NexEngine/Core/IDTypes.h>

namespace Diligent
{
    struct IRenderDevice;
    struct IDeviceContext;
}

namespace Nex
{
    struct PersistentTable
    {
        const char* name;
        std::uint32_t stride;

        std::uint32_t gpu_capacity;
        std::uint32_t logical_capacity;

        std::uint32_t high_water = 0;
        bool gpu_resize_pending = false;

        std::vector<std::byte> mirror; // data
        std::vector<std::uint64_t> dirty_bits;

        std::vector<PersistentSlotIndex> free_slots;
        std::vector<PersistentSlotIndex> freed_this_frame;
        Diligent::GPUCompletionAwaitQueue<std::vector<PersistentSlotIndex>> retire;

        Diligent::DynamicBuffer buffer;
        Diligent::RefCntAutoPtr<Diligent::IBufferView> view;

        PersistentTable(Diligent::IRenderDevice* device,
                        std::uint32_t table_stride,
                        std::uint32_t table_capacity,
                        const char* table_name);

        void GrowLogical(std::uint32_t new_capacity);
    };

    struct PersistentBufferManager
    {
        static constexpr std::uint32_t MaxCapacity = 1u << 24;

        explicit PersistentBufferManager(Diligent::IRenderDevice* device);

        PersistentBufferManager(const PersistentBufferManager&) = delete;
        PersistentBufferManager& operator=(const PersistentBufferManager&) = delete;
        PersistentBufferManager(PersistentBufferManager&&) noexcept = delete;
        PersistentBufferManager& operator=(PersistentBufferManager&&) noexcept = delete;

        PersistentTableID Declare(std::uint32_t stride, std::uint32_t capacity, const char* name);
        PersistentSlotIndex Acquire(PersistentTableID id) const;

        void Write(PersistentTableID id, PersistentSlotIndex slot, const void* data) const;
        void Release(PersistentTableID id, PersistentSlotIndex slot) const;

        void BeginFrame(Diligent::IDeviceContext* context) const;
        void EndFrame(Diligent::IDeviceContext* context) const;

        Diligent::IBufferView* View(PersistentTableID id) const;
        BufferVersion Version(PersistentTableID id) const;
        std::uint32_t Capacity(PersistentTableID id) const;

        template <typename T>
        const T* Read(PersistentTableID id, PersistentSlotIndex slot) const
        {
            const auto* table = Find(id);
            if (!table || slot >= table->high_water || sizeof(T) > table->stride) return nullptr;

            return reinterpret_cast<const T*>(
                table->mirror.data() + static_cast<std::size_t>(slot) * table->stride);
        }

    private:
        PersistentTable* Find(PersistentTableID id) const;
        void FlushDirty(PersistentTable* table, Diligent::IDeviceContext* context) const;

        Diligent::IRenderDevice* device_;

        std::vector<std::unique_ptr<PersistentTable>> tables_;
        Diligent::RefCntAutoPtr<Diligent::IGPUUploadManager> uploads_;
    };
}

#endif //NEXENGINE_PERSISTENTBUFFERMANAGER_H
