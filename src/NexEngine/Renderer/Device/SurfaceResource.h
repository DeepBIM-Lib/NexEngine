//
// Created by SherSpidey on 2026/8/5.
//

#ifndef NEXENGINE_SURFACERESOURCE_H
#define NEXENGINE_SURFACERESOURCE_H

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/Texture.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/SwapChain.h>

namespace Diligent
{
    struct IRenderDevice;
}

namespace Nex
{
    struct Surface;
    struct WindowSurface;
    struct OffscreenSurface;
    struct RenderCore;

    struct SurfaceResource
    {
        // on-screen swap chain
        Diligent::RefCntAutoPtr<Diligent::ISwapChain> swap_chain;

        // off-screen resources
        Diligent::RefCntAutoPtr<Diligent::ITexture> offscreen_color;
        Diligent::RefCntAutoPtr<Diligent::ITexture> offscreen_depth;
        Diligent::RefCntAutoPtr<Diligent::ITextureView> offscreen_rtv;
        Diligent::RefCntAutoPtr<Diligent::ITextureView> offscreen_srv;
        Diligent::RefCntAutoPtr<Diligent::ITextureView> offscreen_dsv;
        Diligent::IRenderDevice* device = nullptr;

        SurfaceResource(const RenderCore& core, const Surface& surface, const WindowSurface& window);
        SurfaceResource(const RenderCore& core, const Surface& surface, const OffscreenSurface& offscreen);

        SurfaceResource(const SurfaceResource&) = delete;
        SurfaceResource& operator=(const SurfaceResource&) = delete;
        SurfaceResource(SurfaceResource&&) noexcept = default;
        SurfaceResource& operator=(SurfaceResource&&) noexcept = default;

        void Resize(std::uint32_t width, std::uint32_t height);

        Diligent::ITextureView* GetRTV() const
        {
            return swap_chain ? swap_chain->GetCurrentBackBufferRTV() : offscreen_rtv;
        }

        Diligent::ITextureView* GetDSV() const
        {
            return swap_chain ? swap_chain->GetDepthBufferDSV() : offscreen_dsv;
        }

        Diligent::TEXTURE_FORMAT GetColorFormat() const
        {
            return swap_chain ? swap_chain->GetDesc().ColorBufferFormat : offscreen_color->GetDesc().Format;
        }

        Diligent::TEXTURE_FORMAT GetDepthFormat() const
        {
            return swap_chain ? swap_chain->GetDesc().DepthBufferFormat : offscreen_depth->GetDesc().Format;
        }

    protected:
        void CreateSwapChain(const RenderCore&, const Surface&, const WindowSurface&);
        void CreateOffscreenResource(
            std::uint32_t width,
            std::uint32_t height,
            Diligent::TEXTURE_FORMAT color_format
        );
    };
}


#endif //NEXENGINE_SURFACERESOURCE_H
