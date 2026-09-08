//
// Created by SherSpidey on 2026/7/10.
//

#include <NexEngine/Renderer/Material/Material.h>
#include <NexEngine/Renderer/Material/MaterialResource.h>
#include <NexEngine/Renderer/Material/MaterialBindingCache.h>

#include <NexEngine/Utils/ResourceManager.h>
#include <NexEngine/Utils/MaterialManager.h>

#include <NexEngine/Renderer/Material/MaterialModule.h>

using namespace Nex;

namespace
{
    template <typename T>
    MaterialDesc CreateMaterialDesc(const Material& material, const T& data)
    {
        MaterialDesc desc;
        desc.material = material;
        desc.model = data;

        return desc;
    }

    void RegisterMaterial(flecs::world& world)
    {
        world.component<AlphaMode>("AlphaMode")
             .constant("Opaque", AlphaMode::Opaque)
             .constant("Mask", AlphaMode::Mask)
             .constant("Blend", AlphaMode::Blend);

        world.component<PBRMaterial>("PBRMaterial")
             .member<Vector4>("BaseColor")
             .member<Vector4>("Emissive")
             .member<float>("Metallic")
             .member<float>("Roughness")
             .member<float>("NormalScale")
             .member<float>("OcclusionStrength")
             .member<String>("BaseTexture")
             .member<String>("NormalTexture")
             .member<String>("OrmTexture")
             .member<String>("EmissiveTexture");

        world.component<PhongMaterial>("PhongMaterial")
             .member<Vector4>("Diffuse")
             .member<Vector4>("Specular")
             .member<Vector4>("Emissive")
             .member<float>("Shininess")
             .member<String>("DiffuseTexture")
             .member<String>("SpecularTexture")
             .member<String>("NormalTexture");

        world.component<MaterialSource>("MaterialSource")
             .add(flecs::Exclusive);

        world.component<MaterialHandle>("MaterialHandle")
             .on_replace([](flecs::entity entity, MaterialHandle& previous, MaterialHandle&)
             {
                 ResourceManager::TryRelease<MaterialManager>(entity, previous);
             })
             .on_remove([](flecs::entity entity, MaterialHandle& handle)
             {
                 ResourceManager::TryRelease<MaterialManager>(entity, handle);
             })
             .member<std::uint32_t>("Index")
             .member<std::uint32_t>("Generation");

        world.component<Material>("Material")
             .member<AlphaMode>("AlphaMode")
             .member<bool>("DoubleSided")
             .member<float>("AlphaCutoff")
             .add(flecs::With, world.component<MaterialHandle>());

        world.observer<MaterialSource>("OnSetMaterialSource")
             .term_at(0).second(flecs::Wildcard)
             .event(flecs::OnSet)
             .each([](flecs::entity entity, MaterialSource)
             {
                 if (const auto material = entity.try_get<Material>())
                 {
                     auto world = entity.world();
                     auto& resource_manager = world.get_mut<ResourceManager>();
                     auto& material_manager = world.get_mut<MaterialManager>();

                     MaterialHandle handle{};

                     auto target = entity.target<MaterialSource>();
                     if (target == world.component<PBRMaterial>())
                     {
                         auto source = static_cast<const PBRMaterial*>(entity.try_get(
                             world.component<MaterialSource>(), target.id()));

                         handle = material_manager.Create(
                             CreateMaterialDesc(*material, *source), resource_manager);
                     }
                     else if (target == world.component<PhongMaterial>())
                     {
                         auto source = static_cast<const PhongMaterial*>(entity.try_get(
                             world.component<MaterialSource>(), target.id()));

                         handle = material_manager.Create(
                             CreateMaterialDesc(*material, *source), resource_manager);
                     }

                     if (IsHandleValid(handle)) entity.set<MaterialHandle>(handle);
                 }
             });

        world.observer<Material>("OnSetMaterial")
             .event(flecs::OnSet)
             .each([](flecs::entity entity, Material& material)
             {
                 auto world = entity.world();
                 auto& resource_manager = world.get_mut<ResourceManager>();
                 auto& material_manager = world.get_mut<MaterialManager>();

                 // create surface resource
                 if (const auto pbr = entity.try_get<MaterialSource, PBRMaterial>())
                 {
                     auto handle = material_manager.Create(
                         CreateMaterialDesc(material, *pbr), resource_manager);

                     if (IsHandleValid(handle)) entity.set<MaterialHandle>(handle);
                 }
                 else if (const auto phong = entity.try_get<MaterialSource, PhongMaterial>())
                 {
                     auto handle = material_manager.Create(
                         CreateMaterialDesc(material, *phong), resource_manager);

                     if (IsHandleValid(handle)) entity.set<MaterialHandle>(handle);
                 }
             });

        world.observer<Material>("OnRemoveMaterial")
             .event(flecs::OnRemove)
             .each([](flecs::entity entity, Material&)
             {
                 entity.remove<MaterialHandle>();
             });
    }
}

MaterialModule::MaterialModule(flecs::world& world)
{
    world.module<MaterialModule>("NexEngine::Material");

    world.component<MaterialBindingCache>("MaterialBindingCache")
         .add(flecs::Singleton);

    RegisterMaterial(world);
}
