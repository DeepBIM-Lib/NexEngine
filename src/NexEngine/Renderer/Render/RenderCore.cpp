//
// Created by SherSpidey on 2026/5/28.
//

#include <unordered_map>

#include <DiligentCore/Primitives/interface/Errors.hpp>

#include <DiligentCore/Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h>
#include <DiligentCore/Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h>
// #include <DiligentCore/Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h>
#include <DiligentCore/Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h>

#include <NexEngine/Renderer/Device/GraphicsAdapterDetails.h>

#include <NexEngine/Renderer/Render/RenderCore.h>

using namespace Nex;
using namespace Diligent;

namespace
{
    std::unordered_map<Render_Device_Type, RENDER_DEVICE_TYPE> device_type_map{
        {Render_Device_Type::D3D11, RENDER_DEVICE_TYPE_D3D11},
        {Render_Device_Type::D3D12, RENDER_DEVICE_TYPE_D3D12},
        {Render_Device_Type::VULKAN, RENDER_DEVICE_TYPE_VULKAN},
    };

    void FillContext(
        const std::vector<IDeviceContext*>& contexts,
        RefCntAutoPtr<IDeviceContext>& context,
        std::vector<RefCntAutoPtr<IDeviceContext>>& deferred_contexts
    )
    {
        if (contexts.empty()) return;

        // setup main context
        context.Attach(contexts[0]);

        // setup deferred contexts
        auto num_deferred = contexts.size();
        for (uint32_t index = 1; index < num_deferred; ++index)
            deferred_contexts.emplace_back().Attach(contexts[index]);
    }

    Uint32 FindAdapter(
        const Uint32& adapter_id,
        ADAPTER_TYPE& adapter_type,
        const IEngineFactory* factory,
        Version graphics_api_version,
        GraphicsAdapterInfo& adapter_attribs)
    {
        // clang-format off
        Uint32 num_adapters = 0;
        factory->EnumerateAdapters(graphics_api_version, num_adapters, nullptr);
        
        std::vector<GraphicsAdapterInfo> adapters(num_adapters);
        if (num_adapters > 0)
            factory->EnumerateAdapters(graphics_api_version, num_adapters, adapters.data());
        else
            LOG_ERROR_AND_THROW("Failed to find compatible hardware adapters");

        auto adapter_id_ = adapter_id;
        
        if (adapter_id_ != DEFAULT_ADAPTER_ID)
        {
            if (adapter_id_ < adapters.size())
            {
                adapter_type = adapters[adapter_id_].Type;
            }
            else
            {
                LOG_ERROR_MESSAGE("Adapter ID (", adapter_id_, ") is invalid. Only ", adapters.size(), " compatible ", (adapters.size() == 1 ? "adapter" : "adapters"), " present in the system");
                adapter_id_ = DEFAULT_ADAPTER_ID;
            }
        }

        if (adapter_id_ == DEFAULT_ADAPTER_ID && adapter_type != ADAPTER_TYPE_UNKNOWN)
        {
            for (Uint32 i = 0; i < adapters.size(); ++i)
            {
                if (adapters[i].Type == adapter_type)
                {
                    adapter_id_ = i;
                    break;
                }
            }
            if (adapter_id_ == DEFAULT_ADAPTER_ID)
                LOG_WARNING_MESSAGE("Unable to find the requested adapter type. Using default adapter.");
        }

        if (adapter_id_ == DEFAULT_ADAPTER_ID)
        {
            adapter_type = ADAPTER_TYPE_UNKNOWN;
            for (Uint32 i = 0; i < adapters.size(); ++i)
            {
                const GraphicsAdapterInfo& adapter_info     = adapters[i];
                const ADAPTER_TYPE         adapter_type_    = adapter_info.Type;
                static_assert((ADAPTER_TYPE_DISCRETE > ADAPTER_TYPE_INTEGRATED &&
                               ADAPTER_TYPE_INTEGRATED > ADAPTER_TYPE_SOFTWARE &&
                               ADAPTER_TYPE_SOFTWARE > ADAPTER_TYPE_UNKNOWN),
                              "Unexpected ADAPTER_TYPE enum ordering");
                if (adapter_type_ > adapter_type)
                {
                    // prefer Discrete over Integrated, over Software
                    adapter_type    = adapter_type_;
                    adapter_id_     = i;
                }
                else if (adapter_type_ == adapter_type)
                {
                    // select adapter with more memory
                    const AdapterMemoryInfo& new_adapter_mem   = adapter_info.Memory;
                    const Uint64             new_total_memory  = new_adapter_mem.LocalMemory + new_adapter_mem.HostVisibleMemory + new_adapter_mem.UnifiedMemory;
                    const AdapterMemoryInfo& curr_adapter_mem  = adapters[adapter_id_].Memory;
                    const Uint64             curr_total_memory = curr_adapter_mem.LocalMemory + curr_adapter_mem.HostVisibleMemory + curr_adapter_mem.UnifiedMemory;
                    if (new_total_memory > curr_total_memory)
                    {
                        adapter_id_ = i;
                    }
                }
            }
        }

        if (adapter_id_ != DEFAULT_ADAPTER_ID)
        {
            adapter_attribs = adapters[adapter_id_];
            LOG_INFO_MESSAGE("Using adapter ", adapter_id_, ": '", adapter_attribs.Description, "'");
        }
        
        // clang-format on

        return adapter_id_;
    }

    template <typename Factory, typename CreateInfo>
    void GetDisplayMode(ADAPTER_TYPE& adapter_type, std::vector<DisplayModeAttribs>& display_modes,
                        Factory* factory, CreateInfo& create_info)
    {
        if (adapter_type != ADAPTER_TYPE_SOFTWARE &&
            create_info.AdapterId != DEFAULT_ADAPTER_ID)
        {
            Uint32 num_display_modes = 0;

            factory->EnumerateDisplayModes(
                create_info.GraphicsAPIVersion, create_info.AdapterId,
                0, TEX_FORMAT_RGBA8_UNORM_SRGB, num_display_modes, nullptr);

            display_modes.resize(num_display_modes);

            factory->EnumerateDisplayModes(
                create_info.GraphicsAPIVersion, create_info.AdapterId,
                0, TEX_FORMAT_RGBA8_UNORM_SRGB, num_display_modes, display_modes.data());
        }
    }
}

RenderCore::RenderCore(const RendererCoreDesc& desc, GraphicsAdapterDetails& adapter_details)
{
#if NDEBUG
    SetDebugMessageCallback([](DEBUG_MESSAGE_SEVERITY,
                               const char*,
                               const char*,
                               const char*,
                               int)
    {
    });
#endif

    auto& adapter_id = adapter_details.adapter_id;
    auto& adapter_type = adapter_details.adapter_type;
    auto& adapter_info = adapter_details.adapter_info;
    auto& display_modes = adapter_details.display_modes;

    device_type = device_type_map[desc.type];
    switch (device_type)
    {
    case RENDER_DEVICE_TYPE_D3D11:
        {
            auto factory_d3d11 = LoadAndGetEngineFactoryD3D11();
            factory.Attach(factory_d3d11);

            EngineD3D11CreateInfo engine_create_info;
            engine_create_info.GraphicsAPIVersion = {11, 0};
            engine_create_info.AdapterId = FindAdapter(adapter_id, adapter_type,
                                                       factory, engine_create_info.GraphicsAPIVersion,
                                                       adapter_info);

            GetDisplayMode(adapter_type, display_modes, factory_d3d11, engine_create_info);

            std::vector<IDeviceContext*> contexts;
            contexts.resize(1 + engine_create_info.NumDeferredContexts);

            factory_d3d11->CreateDeviceAndContextsD3D11(engine_create_info, &device, contexts.data());

            FillContext(contexts, context, deferred_contexts);
        }
        break;
    case RENDER_DEVICE_TYPE_D3D12:
        {
            auto factory_d3d12 = LoadAndGetEngineFactoryD3D12();
            if (!factory_d3d12->LoadD3D12())
                LOG_ERROR_AND_THROW("Failed to load Direct3D12");
            factory.Attach(factory_d3d12);

            EngineD3D12CreateInfo engine_create_info;
            engine_create_info.GraphicsAPIVersion = {11, 0};
            engine_create_info.AdapterId = FindAdapter(adapter_id, adapter_type,
                                                       factory, engine_create_info.GraphicsAPIVersion,
                                                       adapter_info);

            GetDisplayMode(adapter_type, display_modes, factory_d3d12, engine_create_info);

            std::vector<IDeviceContext*> contexts;
            contexts.resize(1 + engine_create_info.NumDeferredContexts);

            factory_d3d12->CreateDeviceAndContextsD3D12(engine_create_info, &device, contexts.data());

            FillContext(contexts, context, deferred_contexts);
        }
        break;
    case RENDER_DEVICE_TYPE_GL:
        {
            // do nothing currently
        }
        break;

    case RENDER_DEVICE_TYPE_VULKAN:
        {
            auto factory_vulkan = LoadAndGetEngineFactoryVk();
            factory.Attach(factory_vulkan);

            EngineVkCreateInfo engine_create_info;
            engine_create_info.AdapterId = FindAdapter(adapter_id, adapter_type,
                                                       factory, engine_create_info.GraphicsAPIVersion,
                                                       adapter_info);

            std::vector<IDeviceContext*> contexts;
            contexts.resize(1 + engine_create_info.NumDeferredContexts);

            factory_vulkan->CreateDeviceAndContextsVk(engine_create_info, &device, contexts.data());

            FillContext(contexts, context, deferred_contexts);
        }
        break;
    default:
        std::cerr << "Unknown/unsupported device type" << '\n';
    }

    factory->CreateDefaultShaderSourceStreamFactory(desc.shader_path, &shader_factory);
}

RenderCore::~RenderCore()
{
    if (device) device->IdleGPU();
}
