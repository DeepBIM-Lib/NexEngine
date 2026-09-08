#define LOCAL_LIGHT_POINT 0u
#define LOCAL_LIGHT_SPOT 1u

struct InstanceRecord
{
    uint transform_index;
    uint material_index;
};

struct TransformData
{
    float4x4 value;
};

struct CameraConstants
{
    float4x4 projection;
    float4x4 view;

    float3 position;
    float padding;
};

struct EnvironmentConstants
{
    float3 ambient_color;
    float ambient_intensity;
};

struct DirectionalLightConstants
{
    float3 direction;
    float intensity;

    float3 color;
    float padding;
};

struct FrameMiscConstants
{
    uint local_light_count;
    uint padding_0;
    uint padding_1;
    uint padding_2;
};

struct LocalLightData
{
    float3 position;
    uint type;


    float3 direction;
    float range;

    float3 color;
    float intensity;

    float spot_inner_cosine;
    float spot_outer_cosine;
    float padding_0;
    float padding_1;
};

StructuredBuffer<InstanceRecord> g_instances;
StructuredBuffer<TransformData> g_transforms;
StructuredBuffer<LocalLightData> g_lights;

cbuffer g_frame_constants
{
    CameraConstants camera;
    FrameMiscConstants frame;
    
    EnvironmentConstants environment;
    DirectionalLightConstants sun;
}

cbuffer g_batch_constants
{
    uint g_instance_base;
    uint batch_padding0;
    uint batch_padding1;
    uint batch_padding2;
}
