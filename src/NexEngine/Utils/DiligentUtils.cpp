//
// Created by SherSpidey on 2026/6/11.
//

#include <DiligentTools/TextureLoader/interface/TextureUtilities.h>

#include <NexEngine/Utils/DiligentUtils.h>

using namespace Nex;
using namespace Diligent;

void Nex::CreateUniformBuffer(IRenderDevice* device,
                              Uint64 size,
                              const char* name,
                              RefCntAutoPtr<IBuffer>& buffer)
{
    // clang-format off
    BufferDesc buffer_desc;
    buffer_desc.Name           = name;
    buffer_desc.Size           = size;
    buffer_desc.Usage          = USAGE_DYNAMIC;
    buffer_desc.BindFlags      = BIND_UNIFORM_BUFFER;
    buffer_desc.CPUAccessFlags = CPU_ACCESS_WRITE;
    // clang-format on

    device->CreateBuffer(buffer_desc, nullptr, &buffer);
}

void Nex::CreateConstantBuffer(IRenderDevice* device,
                               const void* data,
                               Uint64 size,
                               const char* name,
                               RefCntAutoPtr<IBuffer>& buffer)
{
    // clang-format off
    BufferDesc buff_desc;
    buff_desc.Name              = name;
    buff_desc.Usage             = USAGE_DEFAULT;
    buff_desc.BindFlags         = BIND_UNIFORM_BUFFER;
    buff_desc.Size              = size;
    buff_desc.CPUAccessFlags    = CPU_ACCESS_NONE;
    
    BufferData buffer_data;
    buffer_data.pData    = data;
    buffer_data.DataSize = buff_desc.Size;
    // clang-format on

    device->CreateBuffer(buff_desc, &buffer_data, &buffer);
}

RefCntAutoPtr<ITexture> Nex::LoadTexture(IRenderDevice* device,
                                         const char* path,
                                         bool is_srgb)
{
    TextureLoadInfo load_info;
    load_info.IsSRGB = is_srgb;
    RefCntAutoPtr<ITexture> texture;
    CreateTextureFromFile(path, load_info, device, &texture);

    return texture;
}
