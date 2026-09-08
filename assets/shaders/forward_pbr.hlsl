// ReSharper disable CppRedundantCastExpression
#include "forward_shader_layout.hlsl"

static const float PI = 3.14159265359f;
static const float INV_PI = 0.31830988618f;

struct PbrMaterialData
{
    float4 base_color_factor;
    float4 emissive_factor;

    float metallic_factor;
    float roughness_factor;
    float normal_scale;
    float occlusion_strength;

    float alpha_mask;
    float alpha_cutoff;
    float padding_0;
    float padding_1;
};

StructuredBuffer<PbrMaterialData> g_materials;

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

Texture2D g_base_color_texture;
Texture2D g_normal_texture;
Texture2D g_orm_texture;
Texture2D g_emissive_texture;
SamplerState g_base_color_texture_sampler;
SamplerState g_normal_texture_sampler;
SamplerState g_orm_texture_sampler;
SamplerState g_emissive_texture_sampler;

// =================================================================================
// PBR BRDF
// =================================================================================
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float alpha = roughness * roughness;
    float alpha_squared = alpha * alpha;
    float n_dot_h = saturate(dot(N, H));
    float denominator = n_dot_h * n_dot_h * (alpha_squared - 1.0f) + 1.0f;
    return alpha_squared / max(PI * denominator * denominator, 1e-5f);
}

float GeometrySchlickGGX(float n_dot_v, float roughness)
{
    float k = (roughness + 1.0f) * (roughness + 1.0f) / 8.0f;
    return n_dot_v / max(n_dot_v * (1.0f - k) + k, 1e-5f);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    return GeometrySchlickGGX(saturate(dot(N, V)), roughness) *
        GeometrySchlickGGX(saturate(dot(N, L)), roughness);
}

float3 FresnelSchlick(float cos_theta, float3 f0)
{
    return f0 + (1.0f - f0) * pow(saturate(1.0f - cos_theta), 5.0f);
}

float3 ComputePBRDirectLight(float3 N, float3 V, float3 L, float3 radiance, float3 albedo, float roughness,
                             float metallic, float3 f0)
{
    float n_dot_l = saturate(dot(N, L));
    float3 direct_light = float3(0.0f, 0.0f, 0.0f);

    if (n_dot_l > 0.0f)
    {
        float3 H = normalize(V + L);
        float n_dot_v = saturate(dot(N, V));

        float3 fresnel = FresnelSchlick(saturate(dot(H, V)), f0);
        float distribution = DistributionGGX(N, H, roughness);
        float geometry = GeometrySmith(N, V, L, roughness);

        float3 specular = (distribution * geometry * fresnel) / max(4.0f * n_dot_v * n_dot_l, 1e-5f);
        float3 kd = (1.0f - fresnel) * (1.0f - metallic);

        direct_light = (kd * albedo * INV_PI + specular) * radiance * n_dot_l;
    }

    return direct_light;
}

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
    PbrMaterialData m_data = g_materials[input.material_id];

    float4 base_sample = g_base_color_texture.Sample(g_base_color_texture_sampler, input.uv);
    float4 base_color = m_data.base_color_factor * base_sample;
    if (m_data.alpha_mask > 0.5f && base_color.a < m_data.alpha_cutoff)
        discard;

    float face_sign = is_front_face ? 1.0f : -1.0f;
    float3 normal = normalize(input.normal * face_sign);
    float3 tangent = normalize(input.tangent * face_sign);
    float3 bitangent = normalize(cross(normal, tangent) * input.tangent_handedness);

    float3 map_normal = g_normal_texture.Sample(g_normal_texture_sampler, input.uv).xyz * 2.0f - 1.0f;
    map_normal.xy *= m_data.normal_scale;
    map_normal = normalize(map_normal);

    float3 N = normalize(map_normal.x * tangent + map_normal.y * bitangent + map_normal.z * normal);
    float3 V = normalize(input.view_direction);

    float3 orm = g_orm_texture.Sample(g_orm_texture_sampler, input.uv).rgb;
    float roughness = clamp(orm.g * m_data.roughness_factor, 0.04f, 1.0f);
    float metallic = saturate(orm.b * m_data.metallic_factor);
    float ao = lerp(1.0f, orm.r, saturate(m_data.occlusion_strength));

    float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), base_color.rgb, metallic);

    float3 accumulated_lighting = float3(0.0f, 0.0f, 0.0f);

    float3 ambient_radiance = environment.ambient_color * environment.ambient_intensity;
    float3 ambient = ambient_radiance * base_color.rgb * (1.0f - metallic) * ao;
    accumulated_lighting += ambient;

    if (sun.intensity > 0.0f)
    {
        float3 L = normalize(-sun.direction);
        float3 radiance = sun.color * sun.intensity;
        accumulated_lighting += ComputePBRDirectLight(N, V, L, radiance, base_color.rgb, roughness, metallic, f0);
    }

    for (uint i = 0u; i < frame.local_light_count; ++i)
    {
        LocalLightData light = g_lights[i];
        float3 to_light = light.position - input.world_position;
        float dist = length(to_light);

        if (dist > light.range)
            continue;

        float3 L = to_light / max(dist, 1e-4f);

        float dist_factor = dist / max(light.range, 1e-4f);
        float attenuation = max(0.0f, 1.0f - dist_factor * dist_factor);

        if (light.type == 1u)
        {
            float cos_theta = dot(-L, normalize(light.direction));
            float spot_factor = clamp(
                (cos_theta - light.spot_outer_cosine) / max(light.spot_inner_cosine - light.spot_outer_cosine, 1e-4f),
                0.0f, 1.0f);
            attenuation *= (spot_factor * spot_factor);
        }

        if (attenuation > 0.0f)
        {
            float3 radiance = light.color * light.intensity * attenuation;
            accumulated_lighting += ComputePBRDirectLight(N, V, L, radiance, base_color.rgb, roughness, metallic, f0);
        }
    }

    float3 emissive = m_data.emissive_factor.rgb * g_emissive_texture.Sample(g_emissive_texture_sampler, input.uv).rgb;

    float3 final_color = accumulated_lighting + emissive;
    output.color = float4(final_color, base_color.a);
}
