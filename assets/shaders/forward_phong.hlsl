// ReSharper disable CppRedundantCastExpression
#include "forward_shader_layout.hlsl"

static const float PI = 3.14159265359f;
static const float INV_PI = 0.31830988618f;

struct PhongMaterialData
{
    float4 diffuse_color;
    float4 specular_color;
    float4 emissive_color;
    float shininess;
    float alpha_mask;
    float alpha_cutoff;
    float padding;
};

StructuredBuffer<PhongMaterialData> g_materials;

struct VSInput
{
    float3 position : ATTRIB0;
    float3 normal : ATTRIB1;
    float2 uv : ATTRIB2;
    float4 tangent : ATTRIB3;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 world_position : PS_WORLD_POS;
    float2 uv : PS_UV;
    float3 normal : PS_NORMAL;
    float3 tangent : PS_TANGENT;
    float tangent_handedness : PS_HANDEDNESS;
    float3 view_direction : PS_VIEW_DIRECTION;
    nointerpolation uint material_id : PS_MATERIAL_ID;
};

struct PSOutput
{
    float4 color : SV_TARGET;
};

Texture2D g_diffuse_texture;
Texture2D g_specular_texture;
Texture2D g_normal_texture;
SamplerState g_diffuse_texture_sampler;
SamplerState g_specular_texture_sampler;
SamplerState g_normal_texture_sampler;

void VSMain(in VSInput input, uint instance_id : SV_InstanceID, out PSInput output)
{
    InstanceRecord record = g_instances[g_instance_base + instance_id];
    float4x4 model_transform = g_transforms[record.transform_index].value;

    float4 world = mul(float4(input.position, 1.0f), model_transform);

    float3x3 model = (float3x3)model_transform;
    float3x3 cofactor = float3x3(cross(model[1], model[2]),
                                 cross(model[2], model[0]),
                                 cross(model[0], model[1]));
    float determinant = dot(model[0], cross(model[1], model[2]));
    float3x3 inv_model = transpose(cofactor) * rcp(determinant + (determinant < 0.0f ? -1e-6f : 1e-6f));

    float3 normal = normalize(mul(input.normal, inv_model));
    float3 tangent = normalize(mul(input.tangent.xyz, model));
    tangent = normalize(tangent - normal * dot(normal, tangent));

    output.position = mul(mul(world, camera.view), camera.projection);
    output.world_position = world.xyz;
    output.uv = input.uv;
    output.normal = normal;
    output.tangent = tangent;
    output.tangent_handedness = input.tangent.w;
    output.view_direction = normalize(camera.position - world.xyz);
    output.material_id = record.material_index;
}

void PSMain(in PSInput input, bool is_front_face : SV_IsFrontFace, out PSOutput output)
{
    PhongMaterialData m_data = g_materials[input.material_id];

    float4 base_color = m_data.diffuse_color * g_diffuse_texture.Sample(g_diffuse_texture_sampler, input.uv);
    if (m_data.alpha_mask > 0.5f && base_color.a < m_data.alpha_cutoff)
        discard;

    float face_sign = is_front_face ? 1.0f : -1.0f;
    float3 normal = normalize(input.normal * face_sign);
    float3 tangent = normalize(input.tangent);
    float3 bitangent = normalize(cross(normal, tangent) * input.tangent_handedness);
    float3 tangent_normal = g_normal_texture.Sample(g_normal_texture_sampler, input.uv).xyz * 2.0f - 1.0f;

    float3 N = normalize(tangent_normal.x * tangent + tangent_normal.y * bitangent + tangent_normal.z * normal);
    float3 V = normalize(input.view_direction);

    float3 specular_sample = g_specular_texture.Sample(g_specular_texture_sampler, input.uv).rgb;
    float3 spec_tint = m_data.specular_color.rgb * specular_sample;

    float3 diffuse_brdf = base_color.rgb * INV_PI;

    float shininess = max(m_data.shininess, 1.0f);
    float norm_factor = (shininess + 8.0f) * (INV_PI * 0.125f);

    float3 ambient = environment.ambient_color * environment.ambient_intensity * base_color.rgb;
    float3 accumulated_lighting = float3(0.0f, 0.0f, 0.0f);

    if (sun.intensity > 0.0f)
    {
        float3 L = normalize(-sun.direction);
        float n_dot_l = max(dot(N, L), 0.0f);
        if (n_dot_l > 0.0f)
        {
            float3 H = normalize(L + V);
            float spec = pow(max(dot(N, H), 0.0f), shininess) * norm_factor;
            float3 specular_brdf = spec_tint * spec;
            float3 radiance = sun.color * sun.intensity;

            accumulated_lighting += (diffuse_brdf + specular_brdf) * n_dot_l * radiance;
        }
    }

    for (uint i = 0u; i < frame.local_light_count; ++i)
    {
        LocalLightData light = g_lights[i];
        float3 to_light = light.position - input.world_position;
        float dist = length(to_light);

        if (dist > light.range)
            continue;

        float3 L = to_light / max(dist, 1e-4f);
        float n_dot_l = max(dot(N, L), 0.0f);
        if (n_dot_l <= 0.0f)
            continue;

        float dist_factor = dist / max(light.range, 1e-4f);
        float attenuation = max(0.0f, 1.0f - dist_factor * dist_factor);

        if (light.type == LOCAL_LIGHT_SPOT)
        {
            float cos_theta = dot(-L, normalize(light.direction));
            float spot_factor = clamp(
                (cos_theta - light.spot_outer_cosine) / max(light.spot_inner_cosine - light.spot_outer_cosine, 1e-4f),
                0.0f, 1.0f
            );
            attenuation *= (spot_factor * spot_factor);
        }

        float3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0f), shininess) * norm_factor;
        float3 specular_brdf = spec_tint * spec;
        float3 radiance = light.color * light.intensity * attenuation;

        accumulated_lighting += (diffuse_brdf + specular_brdf) * n_dot_l * radiance;
    }

    float3 final_color = ambient + m_data.emissive_color.rgb + accumulated_lighting;
    output.color = float4(final_color, base_color.a);
}
