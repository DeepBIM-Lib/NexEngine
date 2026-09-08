//
// Created by SherSpidey on 2026/8/5.
//

#include <bit>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h>
#include <DiligentCore/Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h>
#include <DiligentCore/Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h>
// #include <DiligentCore/Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h>
#include <DiligentCore/Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h>

#include <NexEngine/Renderer/Device/Surface.h>
#include <NexEngine/Renderer/Render/RenderCore.h>

#include <NexEngine/Utils/DiligentUtils.h>

#include <NexEngine/Renderer/Device/SurfaceResource.h>

using namespace Nex;
using namespace Diligent;

SurfaceResource::SurfaceResource(
    const RenderCore& core, const Surface& surface, const WindowSurface& window)
{
    device = core.device.RawPtr();
    CreateSwapChain(core, surface, window);
}

SurfaceResource::SurfaceResource(
    const RenderCore& core, const Surface& surface, const OffscreenSurface& offscreen)
{
    device = core.device.RawPtr();
    CreateOffscreenResource(surface.width, surface.height, ToTextureFormat(surface.color_format));
}

void SurfaceResource::Resize(std::uint32_t width, std::uint32_t height)
{
    if (width == 0 || height == 0) return;

    // offscreen texture resize
    if (!swap_chain)
    {
        const auto& pre_color_desc = offscreen_color->GetDesc();
        auto& color_format = pre_color_desc.Format;
        
        CreateOffscreenResource(width, height, color_format);
        
        return;
    }

    // swap chain resize
    swap_chain->Resize(width, height);
}

void SurfaceResource::CreateSwapChain(
    const RenderCore& core, const Surface& surface, const WindowSurface& window)
{
    SwapChainDesc sc_desc;
    sc_desc.Width = surface.width;
    sc_desc.Height = surface.height;
    sc_desc.ColorBufferFormat = ToTextureFormat(surface.color_format);
    sc_desc.DepthBufferFormat = TEX_FORMAT_D24_UNORM_S8_UINT;
    sc_desc.IsPrimary = false;

    swap_chain.Release();

    NativeWindow native_window;
#ifdef _WIN32
    native_window.hWnd = std::bit_cast<HWND>(window.w_id);
#endif

    switch (core.device_type)
    {
    case RENDER_DEVICE_TYPE_D3D11:
        {
            const auto factory = core.factory.RawPtr<IEngineFactoryD3D11>();
            factory->CreateSwapChainD3D11(
                core.device, core.context, sc_desc, FullScreenModeDesc{},
                native_window, &swap_chain
            );
        }
        break;
    case RENDER_DEVICE_TYPE_D3D12:
        {
            const auto factory = core.factory.RawPtr<IEngineFactoryD3D12>();
            factory->CreateSwapChainD3D12(
                core.device, core.context, sc_desc, FullScreenModeDesc{},
                native_window, &swap_chain
            );
        }
        break;
    case RENDER_DEVICE_TYPE_GL:
        {
            // do nothing currently
        }
        break;

    case RENDER_DEVICE_TYPE_VULKAN:
        {
            const auto factory = core.factory.RawPtr<IEngineFactoryVk>();
            factory->CreateSwapChainVk(
                core.device, core.context, sc_desc, native_window, &swap_chain
            );
        }
        break;
    default:
        std::cerr << "Unknown/unsupported device type" << '\n';
    }
}

void SurfaceResource::CreateOffscreenResource(
    std::uint32_t width,
    std::uint32_t height,
    TEXTURE_FORMAT color_format)
{
    offscreen_color.Release();
    offscreen_depth.Release();
    offscreen_rtv.Release();
    offscreen_srv.Release();
    offscreen_dsv.Release();

    if (!device) return;

    TextureDesc color_desc;
    color_desc.Name = "Nex Offscreen Color";
    color_desc.Type = RESOURCE_DIM_TEX_2D;
    color_desc.Width = width;
    color_desc.Height = height;
    color_desc.Format = color_format;
    color_desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
    device->CreateTexture(color_desc, nullptr, &offscreen_color);

    TextureDesc depth_desc;
    depth_desc.Name = "Nex Offscreen Depth";
    depth_desc.Type = RESOURCE_DIM_TEX_2D;
    depth_desc.Width = width;
    depth_desc.Height = height;
    depth_desc.Format = TEX_FORMAT_D24_UNORM_S8_UINT;
    depth_desc.BindFlags = BIND_DEPTH_STENCIL;
    device->CreateTexture(depth_desc, nullptr, &offscreen_depth);

    if (!offscreen_color || !offscreen_depth) return;

    offscreen_rtv = offscreen_color->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    offscreen_srv = offscreen_color->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    offscreen_dsv = offscreen_depth->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
}
