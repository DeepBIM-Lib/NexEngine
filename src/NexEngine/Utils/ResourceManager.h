//
// Created by SherSpidey on 2026/7/9.
//

#ifndef NEXENGINE_RESOURCEMANAGER_H
#define NEXENGINE_RESOURCEMANAGER_H

#include <utility>
#include <typeindex>
#include <unordered_map>

#include <flecs.h>

#include <NexEngine/Core/HandleTable.h>
#include <NexEngine/App/EngineContext.h>

namespace Nex
{
    struct ResourceManager
    {
        ResourceManager() = default;

        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;
        ResourceManager(ResourceManager&&) noexcept = delete;
        ResourceManager& operator=(ResourceManager&&) noexcept = delete;

        template <typename T>
        void RegisterResource()
        {
            if (const auto type_index = std::type_index(typeid(T)); !handle_tables_.contains(type_index))
            {
                handle_tables_[type_index] = std::make_unique<HandleTable<T>>();
            }
        }

        template <typename T, typename... Args>
        Handle<T> CreateResource(Args&&... args)
        {
            const auto type_index = std::type_index(typeid(T));

            auto it = handle_tables_.find(type_index);
            if (it == handle_tables_.end())
            {
                RegisterResource<T>();
                it = handle_tables_.find(type_index);
            }

            auto handle_table = static_cast<HandleTable<T>*>(it->second.get());
            return handle_table->Create(std::forward<Args>(args)...);
        }

        template <typename T>
        const T* GetResource(Handle<T> handle) const
        {
            const auto type_index = std::type_index(typeid(T));
            const auto it = handle_tables_.find(type_index);
            if (it != handle_tables_.end())
            {
                const auto handle_table = static_cast<const HandleTable<T>*>(it->second.get());
                return handle_table->TryGet(handle);
            }
            return nullptr;
        }

        template <typename T>
        T* GetResource(Handle<T> handle)
        {
            return const_cast<T*>(std::as_const(*this).GetResource<T>(handle));
        }

        template <typename T>
        bool DestroyResource(Handle<T> handle, std::uint64_t retire_serial)
        {
            const auto type_index = std::type_index(typeid(T));
            const auto it = handle_tables_.find(type_index);
            if (it != handle_tables_.end())
            {
                auto handle_table = static_cast<HandleTable<T>*>(it->second.get());
                return handle_table->Destroy(handle, retire_serial);
            }
            return false;
        }

        template <typename T>
        const HandleTable<T>* GetResources() const
        {
            const auto type_index = std::type_index(typeid(T));
            const auto it = handle_tables_.find(type_index);
            if (it != handle_tables_.end())
                return static_cast<HandleTable<T>*>(it->second.get());

            return nullptr;
        }
        
        template <typename T>
        bool Retain(Handle<T> handle)
        {
            auto resource = GetResource(handle);
            if (!resource) return false;
            
            constexpr auto max_ref = (std::numeric_limits<std::uint32_t>::max)();
            if (resource->ref_count == max_ref) return false;

            ++resource->ref_count;
            return true;
        }
        
        template <typename T>
        bool ValidateRetain(Handle<T> handle, std::uint32_t count)
        {
            auto resource = GetResource(handle);
            if (!resource) return false;
            
            constexpr auto max_ref = (std::numeric_limits<std::uint32_t>::max)();
            if (count > max_ref - resource->ref_count) return false;

            return true;
        }

        template <typename T, typename Func>
        void ForEachResource(Func&& func) const
        {
            const auto type_index = std::type_index(typeid(T));
            if (auto it = handle_tables_.find(type_index); it != handle_tables_.end())
            {
                auto table = static_cast<HandleTable<T>*>(it->second.get());
                table->Each(std::forward<Func>(func));
            }
        }

        void Collect(std::uint64_t completed_serial)
        {
            for (const auto& [_, handle_table] : handle_tables_)
                handle_table->Collect(completed_serial);
        }
        
        template<typename M, typename T>
        bool Release(Handle<T> handle, M& manager, EngineContext& context)
        {
            bool retired = false;
            
            const bool released = manager.Release(handle, *this, context.render_frame_index + 1, retired);
            context.retirement_signal_requested |= retired;
            
            return released;
        }
        
        template<typename M, typename T>
        static void TryRelease(flecs::entity entity, Handle<T> handle)
        {
            if (!IsHandleValid(handle)) return;
            
            auto world = entity.world();

            auto& engine_context = world.get_mut<EngineContext>();
            auto& resource_manager = world.get_mut<ResourceManager>();
            auto& manager = world.get_mut<M>();
            
            // try to release resource
            resource_manager.Release(handle, manager, engine_context);
        }

    private:
        std::unordered_map<std::type_index, std::unique_ptr<IHandleTable>> handle_tables_;
    };
}

#endif //NEXENGINE_RESOURCEMANAGER_H
