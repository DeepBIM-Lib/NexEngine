#include "forward_shader_layout.hlsl"

cbuffer TopologyOverlayConstants
{
    float4 overlay_color;
    float4 overlay_depth_parameters;
}

struct VSInput
{
    float3 position : ATTRIB0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float4 depth_parameters : TEXCOORD0;
};

struct PSInput
{
    noperspective centroid float4 position : SV_POSITION;
    nointerpolation float4 color : COLOR0;
    nointerpolation float4 depth_parameters : TEXCOORD0;
};

void VSMain(in VSInput input, out VSOutput output)
{
    float4 world = mul(float4(input.position, 1.0f), transform);
    float4 clip = mul(mul(world, camera_view), camera_project);
    output.position = clip;
    output.color = overlay_color;
    output.depth_parameters = overlay_depth_parameters;
}

void PSMain(
    in PSInput input,
    out float4 color : SV_TARGET,
    out float output_depth : SV_DepthLessEqual
)
{
    float forward_offset = input.depth_parameters.x;
    if (input.depth_parameters.y > 0.0f)
    {
        // Line and triangle rasterization can evaluate a shared geometric edge
        // at different sample positions. Bound that screen-space discrepancy by
        // the local half-pixel depth gradient, then preserve real occluders with
        // the CPU-provided maximum offset.
        float half_pixel_slope = 0.5f * (
            abs(ddx(input.position.z)) + abs(ddy(input.position.z))
        );
        forward_offset = min(
            forward_offset + half_pixel_slope * input.depth_parameters.y,
            input.depth_parameters.z
        );
    }
    output_depth = max(0.0f, input.position.z - forward_offset);
    color = input.color;
}
