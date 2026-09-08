//
// Created by SherSpidey on 2026/5/26.
//

#ifndef NEXENGINE_HANDLETABLE_H
#define NEXENGINE_HANDLETABLE_H

#include <deque>
#include <memory>
#include <thread>
#include <utility>
#include <vector>
#include <cassert>

#include <NexEngine/Core/Handle.h>

namespace Nex
{
    template <typename T>
    struct DefaultHandleDestroyPolicy
    {
        void operator()(T&) const noexcept
        {
            // default do nothing
        }
    };

    struct IHandleTable
    {
        virtual ~IHandleTable() = default;

        virtual void Collect(std::uint64_t completed_serial) = 0;
    };

    template <typename Type, typename DestroyPolicy = DefaultHandleDestroyPolicy<Type>>
    class HandleTable : public IHandleTable
    {
        using Handle = Handle<Type>;

    public:
        struct Stats
        {
            std::size_t live_count = 0;
            std::size_t peak_live_count = 0;
            std::size_t pending_destroy_count = 0;

            // lifetime counters
            std::size_t total_created = 0;
            std::size_t total_destroyed = 0;
        };

        HandleTable() = default;

        ~HandleTable() override
        {
            AssertOwnerThread();
            ClearImmediate();
        }

        explicit HandleTable(DestroyPolicy policy) :
            destroy_policy_(std::move(policy))
        {
        }

        HandleTable(const HandleTable&) = delete;
        HandleTable& operator=(const HandleTable&) = delete;
        HandleTable(HandleTable&&) = delete;
        HandleTable& operator=(HandleTable&&) = delete;

        void Reserve(std::size_t capacity)
        {
            AssertOwnerThread();

            slots_.reserve(capacity);
            free_list_.reserve(capacity);
        }

        template <typename... Args>
        Handle Create(Args&&... args)
        {
            AssertOwnerThread();

            return InsertToSlot(std::make_unique<Type>(std::forward<Args>(args)...));
        }

        bool Destroy(Handle handle, std::uint64_t retire_serial)
        {
            AssertOwnerThread();

            if (!IsHandleValid(handle))
            {
                assert(false && "Destroy called with invalid handle.");
                return false;
            }

            if (handle.index >= slots_.size())
            {
                assert(false && "Destroy called with out-of-range handle.");
                return false;
            }

            auto& slot = slots_[handle.index];

            if (slot.state != SlotState::Live || !slot.object)
            {
                assert(false && "Destroy called on non-live slot.");
                return false;
            }

            if (slot.generation != handle.generation)
            {
                assert(false && "Destroy called with stale handle.");
                return false;
            }

            auto next_generation = slot.generation;
            if (!AdvanceGeneration(next_generation))
            {
                assert(false && "Live slot has an invalid generation.");
                return false;
            }

            // reserve the pending-destroy node before detaching the live object from its slot
            pending_destroy_.emplace_back();
            auto& pending = pending_destroy_.back();
            pending.retire_serial = retire_serial;
            pending.object = std::move(slot.object);

            slot.state = SlotState::Free;
            slot.generation = next_generation;
            free_list_.push_back(handle.index);

            --stats_.live_count;
            ++stats_.pending_destroy_count;

            return true;
        }

        Type* TryGet(Handle handle) noexcept
        {
            AssertOwnerThread();

            if (!IsHandleValid(handle))
                return nullptr;

            if (handle.index >= slots_.size())
                return nullptr;

            auto& slot = slots_[handle.index];
            if (slot.state != SlotState::Live || !slot.object)
                return nullptr;

            if (slot.generation != handle.generation)
                return nullptr;

            return slot.object.get();
        }

        const Type* TryGet(Handle handle) const noexcept
        {
            AssertOwnerThread();

            if (!IsHandleValid(handle))
                return nullptr;

            if (handle.index >= slots_.size())
                return nullptr;

            const auto& slot = slots_[handle.index];
            if (slot.state != SlotState::Live || !slot.object)
                return nullptr;

            if (slot.generation != handle.generation)
                return nullptr;

            return slot.object.get();
        }

        Type& GetChecked(Handle handle)
        {
            auto object = TryGet(handle);
            if (!object)
            {
                assert(false && "GetChecked failed: handle does not reference a live object.");
                std::abort();
            }
            return *object;
        }

        const Type& GetChecked(Handle handle) const
        {
            const auto object = TryGet(handle);
            if (!object)
            {
                assert(false && "GetChecked failed: handle does not reference a live object.");
                std::abort();
            }
            return *object;
        }

        template <typename Func>
        void Each(Func&& func)
        {
            AssertOwnerThread();

            for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(slots_.size()); ++i)
            {
                auto& slot = slots_[i];

                if (slot.state == SlotState::Live && slot.object)
                {
                    func(*slot.object);
                }
            }
        }

        // serial usually comes from frame fence / submit serial
        void Collect(std::uint64_t completed_serial) override
        {
            AssertOwnerThread();

            while (!pending_destroy_.empty())
            {
                PendingDestroy& pending = pending_destroy_.front();
                if (pending.retire_serial > completed_serial)
                    break;

                assert(pending.object);

                destroy_policy_(*pending.object);
                pending.object.reset();
                pending_destroy_.pop_front();

                --stats_.pending_destroy_count;

                ++stats_.total_destroyed;
            }
        }

        void ClearImmediate()
        {
            AssertOwnerThread();

            std::size_t destroyed_now = 0;

            for (auto& slot : slots_)
            {
                const bool was_live = slot.state == SlotState::Live && slot.object != nullptr;

                if (slot.object)
                {
                    destroy_policy_(*slot.object);
                    slot.object.reset();
                    ++destroyed_now;
                }

                slot.state = SlotState::Free;

                if (was_live)
                {
                    const bool advanced = AdvanceGeneration(slot.generation);
                    assert(advanced && "Live slot has an invalid generation during ClearImmediate.");
                }
            }

            while (!pending_destroy_.empty())
            {
                auto& pending = pending_destroy_.front();
                if (pending.object)
                {
                    destroy_policy_(*pending.object);
                    pending.object.reset();
                    ++destroyed_now;
                }
                pending_destroy_.pop_front();
            }

            RebuildFreeList();

            stats_.live_count = 0;
            stats_.pending_destroy_count = 0;

            stats_.total_destroyed += destroyed_now;
        }

        const Stats& GetStats() const noexcept
        {
            return stats_;
        }

        std::size_t SlotCount() const noexcept
        {
            return slots_.size();
        }

    private:
        static constexpr std::uint32_t InitialGeneration = 1;

        enum class SlotState : std::uint8_t
        {
            Free,
            Live
        };

        struct Slot
        {
            std::unique_ptr<Type> object;
            std::uint32_t generation = InitialGeneration;
            SlotState state = SlotState::Free;
        };

        struct PendingDestroy
        {
            std::unique_ptr<Type> object;
            std::uint64_t retire_serial = 0;
        };

        static Handle MakeHandle(std::uint32_t index, std::uint32_t generation) noexcept
        {
            return Handle{index, generation};
        }

        Handle InsertToSlot(std::unique_ptr<Type> object)
        {
            std::uint32_t index = 0;
            bool reused_slot = false;
            // try to resue slot
            while (!free_list_.empty())
            {
                index = free_list_.back();
                free_list_.pop_back();

                auto& slot = slots_[index];
                // retired slot, abandon
                if (slot.generation == InvalidHandleGeneration)
                    continue;

                assert(slot.state == SlotState::Free);
                assert(!slot.object);

                slot.object = std::move(object);
                slot.state = SlotState::Live;
                reused_slot = true;
                break;
            }

            // insert new slot
            if (!reused_slot)
            {
                index = static_cast<std::uint32_t>(slots_.size());

                slots_.push_back(
                    Slot{
                        .object = std::move(object),
                        .generation = InitialGeneration,
                        .state = SlotState::Live
                    }
                );
            }

            ++stats_.live_count;
            ++stats_.total_created;

            if (stats_.live_count > stats_.peak_live_count)
                stats_.peak_live_count = stats_.live_count;

            return MakeHandle(index, slots_[index].generation);
        }

        static bool AdvanceGeneration(std::uint32_t& generation) noexcept
        {
            if (generation == InvalidHandleGeneration) return false;

            ++generation;
            return true;
        }

        void AssertOwnerThread() const noexcept
        {
#ifndef NDEBUG
            assert(owner_thread_ == std::this_thread::get_id() &&
                "HandleTable must only be accessed from its owner thread.");
#endif
        }

        void RebuildFreeList()
        {
            free_list_.clear();
            free_list_.reserve(slots_.size());

            for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(slots_.size()); ++i)
            {
                const auto& slot = slots_[i];

                if (slot.state != SlotState::Free)
                    continue;

                if (slot.generation == InvalidHandleGeneration)
                    continue;

                free_list_.push_back(i);
            }
        }

        Stats stats_;
        DestroyPolicy destroy_policy_;

        std::vector<Slot> slots_;
        std::vector<std::uint32_t> free_list_;
        std::deque<PendingDestroy> pending_destroy_;

#ifndef NDEBUG
        std::thread::id owner_thread_ = std::this_thread::get_id();
#endif
    };
}

#endif //NEXENGINE_HANDLETABLE_H
