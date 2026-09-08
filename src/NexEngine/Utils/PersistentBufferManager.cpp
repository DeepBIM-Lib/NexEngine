//
// Created by SherSpidey on 2026/8/21.
//

#include <NexEngine/Utils/DiligentUtils.h>
#include <NexEngine/Utils/PersistentBufferManager.h>

using namespace Nex;
using namespace Diligent;

namespace
{
}

PersistentTable::PersistentTable(IRenderDevice* device,
                                 std::uint32_t table_stride,
                                 std::uint32_t table_capacity,
                                 const char* table_name)
    : name{table_name},
      stride{table_stride},
      gpu_capacity{table_capacity},
      logical_capacity{table_capacity},
      retire{device},
      buffer{device, DynamicBufferCreateInfo{MakeStructuredBufferDesc(table_name, table_stride, table_capacity)}}
{
    // reset data
    mirror.assign(static_cast<std::size_t>(table_capacity) * stride, std::byte{0});
    dirty_bits.assign((table_capacity + 63) / 64, 0);

    if (const auto gpu_buffer = buffer.GetBuffer())
    {
        view = RefCntAutoPtr(gpu_buffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
    }
}

void PersistentTable::GrowLogical(std::uint32_t new_capacity)
{
    auto new_size = static_cast<std::size_t>(new_capacity) * stride;

    mirror.resize(new_size, std::byte{0});
    dirty_bits.resize((new_capacity + 63) / 64, 0);
    logical_capacity = new_capacity;
    gpu_resize_pending = gpu_capacity < new_capacity;
}

PersistentBufferManager::PersistentBufferManager(IRenderDevice* device) : device_{device}
{
    GPUUploadManagerCreateInfo info;
    info.pDevice = device_;
    info.PageSize = 256 << 10; // 256 KB per upload page
    info.LargePageSize = 1 << 20; // 1 MB per large upload page
    info.InitialPageCount = 2;
    info.MaxPageCount = 16;
    CreateGPUUploadManager(info, &uploads_);

    if (!uploads_)
        LOG_ERROR_MESSAGE("PersistentBufferManager: failed to create GPUUploadManager");
}

PersistentTableID PersistentBufferManager::Declare(std::uint32_t stride,
                                                   std::uint32_t capacity,
                                                   const char* name)
{
    if (!device_ || stride == 0 || capacity == 0 || !name)
    {
        LOG_ERROR_MESSAGE("PersistentBufferManager: invalid declare arguments");
        return InvalidPersistentTableID;
    }

    tables_.push_back(std::make_unique<PersistentTable>(device_, stride, capacity, name));
    return static_cast<PersistentTableID>(tables_.size());
}

PersistentSlotIndex PersistentBufferManager::Acquire(PersistentTableID id) const
{
    auto table = Find(id);
    if (!table) return InvalidPersistentSlotIndex;

    if (!table->free_slots.empty())
    {
        const auto slot = table->free_slots.back();
        table->free_slots.pop_back();

        return slot;
    }

    if (table->high_water >= MaxCapacity)
    {
        LOG_ERROR_MESSAGE("PersistentBufferManager: table '", table->name, "' exceeded capacity limit");
        return InvalidPersistentSlotIndex;
    }

    // expanding
    if (table->high_water == table->logical_capacity)
    {
        table->GrowLogical(table->logical_capacity * 2);
    }

    return table->high_water++;
}

void PersistentBufferManager::Write(PersistentTableID id, PersistentSlotIndex slot, const void* data) const
{
    auto table = Find(id);
    if (!table) return;

    if (slot >= table->high_water || !data)
    {
        LOG_ERROR_MESSAGE("PersistentBufferManager: bad slot ", slot, " in table '", table->name, "'");
        return;
    }

    const auto dst = table->mirror.data() + static_cast<std::size_t>(slot) * table->stride;
    if (std::memcmp(dst, data, table->stride) == 0) return;
    
    std::memcpy(dst, data, table->stride);

    table->dirty_bits[slot >> 6] |= 1ull << (slot & 63);
}

void PersistentBufferManager::Release(PersistentTableID id, PersistentSlotIndex slot) const
{
    PersistentTable* table = Find(id);
    if (!table) return;

    if (slot >= table->high_water)
    {
        LOG_ERROR_MESSAGE("PersistentBufferManager: slot ", slot, " was never acquired in table '", table->name, "'");
        return;
    }

    table->freed_this_frame.push_back(slot);
}

void PersistentBufferManager::BeginFrame(IDeviceContext* context) const
{
    for (auto& table : tables_)
    {
        for (auto batch = table->retire.GetFirstCompleted(); !batch.empty(); batch = table->retire.GetFirstCompleted())
        {
            table->free_slots.insert(table->free_slots.end(), batch.begin(), batch.end());
        }

        if (table->gpu_resize_pending)
        {
            auto new_size = static_cast<Uint64>(table->logical_capacity) * table->stride;
            table->buffer.Resize(device_, context, new_size);

            if (IBuffer* gpu = table->buffer.GetBuffer())
            {
                table->view = RefCntAutoPtr(gpu->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
            }

            table->gpu_capacity = table->logical_capacity;
            table->gpu_resize_pending = false;
        }

        FlushDirty(table.get(), context);
    }

    uploads_->RenderThreadUpdate(context);
}

void PersistentBufferManager::EndFrame(IDeviceContext* context) const
{
    for (auto& table : tables_)
    {
        if (table->freed_this_frame.empty()) continue;

        table->retire.Enqueue(context, std::move(table->freed_this_frame));
    }
}

IBufferView* PersistentBufferManager::View(PersistentTableID id) const
{
    auto table = Find(id);
    return table ? table->view.RawPtr() : nullptr;
}

BufferVersion PersistentBufferManager::Version(PersistentTableID id) const
{
    auto table = Find(id);
    return table ? table->buffer.GetVersion() : InvalidBufferVersion;
}

std::uint32_t PersistentBufferManager::Capacity(PersistentTableID id) const
{
    auto* table = Find(id);
    return table ? table->logical_capacity : 0;
}

PersistentTable* PersistentBufferManager::Find(PersistentTableID id) const
{
    if (id == InvalidPersistentTableID || id > tables_.size())
    {
        LOG_ERROR_MESSAGE("PersistentBufferManager: invalid PersistentTableID ", id);
        return nullptr;
    }

    return tables_[id - 1].get();
}

void PersistentBufferManager::FlushDirty(PersistentTable* table, IDeviceContext* context) const
{
    auto* dst = table->buffer.GetBuffer();
    const auto scan_words = (table->high_water + 63) / 64;
    auto run_start = InvalidPersistentSlotIndex;

    for (std::uint32_t word_index = 0; word_index <= scan_words; ++word_index)
    {
        const auto word = word_index < scan_words ? table->dirty_bits[word_index] : 0;

        if (word == 0 && run_start == InvalidPersistentSlotIndex)
        {
            if (word_index < scan_words) table->dirty_bits[word_index] = 0;
            continue;
        }

        for (std::uint32_t bit_index = 0; bit_index < 64; ++bit_index)
        {
            if (word & (1ull << bit_index))
            {
                if (run_start == InvalidPersistentSlotIndex) run_start = word_index * 64 + bit_index;
            }
            else if (run_start != InvalidPersistentSlotIndex)
            {
                const auto count = word_index * 64 + bit_index - run_start;

                uploads_->ScheduleBufferUpdate(
                    {
                        context,
                        dst,
                        run_start * table->stride,
                        count * table->stride,
                        table->mirror.data() + static_cast<std::size_t>(run_start) * table->stride
                    }
                );

                run_start = InvalidPersistentSlotIndex;
            }
        }

        if (word_index < scan_words) table->dirty_bits[word_index] = 0;
    }
}
