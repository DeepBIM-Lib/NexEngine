//
// Created by SherSpidey on 2026/6/11.
//

#ifndef NEXENGINE_DEVICEUTILS_H
#define NEXENGINE_DEVICEUTILS_H

#include <vector>

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/GraphicsTypes.h>

#include <NexEngine/Renderer/Device/Surface.h>
#include <NexEngine/Renderer/Renderer.h>

namespace Nex
{
    void CreateUniformBuffer(Diligent::IRenderDevice* device,
                             Diligent::Uint64 size,
                             const char* name,
                             Diligent::RefCntAutoPtr<Diligent::IBuffer>& buffer);

    void CreateConstantBuffer(Diligent::IRenderDevice* device,
                              const void* data,
                              Diligent::Uint64 size,
                              const char* name,
                              Diligent::RefCntAutoPtr<Diligent::IBuffer>& buffer);

    template <typename T>
    void CreateImmutableBuffer(Diligent::IRenderDevice* device,
                               const std::vector<T>& data,
                               const char* name,
                               Diligent::BIND_FLAGS flag,
                               Diligent::RefCntAutoPtr<Diligent::IBuffer>& buffer)
    {
        // clang-format off
        Diligent::BufferDesc buff_desc;
        buff_desc.Name      = name;
        buff_desc.Usage     = Diligent::USAGE_IMMUTABLE;
        buff_desc.BindFlags = flag;
        buff_desc.Size      = data.size() * sizeof(T);
    
        Diligent::BufferData buffer_data;
        buffer_data.pData    = data.data();
        buffer_data.DataSize = buff_desc.Size;
        // clang-format on

        device->CreateBuffer(buff_desc, &buffer_data, &buffer);
    }

    inline Diligent::BufferDesc MakeStructuredBufferDesc(const char* name,
                                                         std::uint32_t stride,
                                                         std::uint32_t capacity)
    {
        Diligent::BufferDesc desc;
        desc.Name = name;
        desc.Size = static_cast<Diligent::Uint64>(stride) * capacity;
        desc.BindFlags = Diligent::BIND_SHADER_RESOURCE;
        desc.Usage = Diligent::USAGE_DEFAULT;
        desc.Mode = Diligent::BUFFER_MODE_STRUCTURED;
        desc.ElementByteStride = stride;
        return desc;
    }

    Diligent::RefCntAutoPtr<Diligent::ITexture> LoadTexture(Diligent::IRenderDevice* device,
                                                            const char* path,
                                                            bool is_srgb = true);

    inline Diligent::TEXTURE_FORMAT ToTextureFormat(const ColorFormat color_format)
    {
        switch (color_format)
        {
        case ColorFormat::RGBA8_UNORM_SRGB:
            return Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB;
        case ColorFormat::RGBA8_UNORM:
            return Diligent::TEX_FORMAT_RGBA8_UNORM;
        case ColorFormat::BGRA8_UNORM_SRGB:
            return Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB;
        case ColorFormat::BGRA8_UNORM:
            return Diligent::TEX_FORMAT_BGRA8_UNORM;
        case ColorFormat::RGBA16_FLOAT:
            return Diligent::TEX_FORMAT_RGBA16_FLOAT;
        }
        return Diligent::TEX_FORMAT_UNKNOWN;
    }

    inline Diligent::PRIMITIVE_TOPOLOGY ToPrimitiveTopology(RenderTopoFlag flag)
    {
        if (flag == RenderTopoFlag::Line) return Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST;
        if (flag == RenderTopoFlag::Point) return Diligent::PRIMITIVE_TOPOLOGY_POINT_LIST;

        return Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}


#endif //NEXENGINE_DEVICEUTILS_H
