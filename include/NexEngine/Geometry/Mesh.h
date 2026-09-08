//
// Created by SherSpidey on 2026/7/10.
//

#ifndef NEXENGINE_MESH_H
#define NEXENGINE_MESH_H

#include <NexEngine/Core/Handle.h>
#include <NexEngine/Geometry/MeshData.h>

namespace Nex
{
    using MeshHandle = Handle<struct MeshResource>;
    
    struct MeshDesc
    {
        MeshData data;
        
        // optional
        String cache_key;
    };
}

#endif //NEXENGINE_MESH_H
