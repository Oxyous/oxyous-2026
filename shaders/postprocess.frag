#version 450

layout (set = 0, binding = 0) uniform sampler2D gAlbedo;
layout (set = 0, binding = 1) uniform sampler2D gNormal;
layout (set = 0, binding = 2) uniform sampler2D gPBR;
layout (set = 0, binding = 3) uniform sampler2D gPosition;

layout(set = 0, binding = 4) uniform PostProcessUBO {
    mat4 projection;
    mat4 view;
    mat4 invView;
    vec4 cameraPosition;
} ubo;

layout (location = 0) in vec2 uvCoord;
layout (location = 0) out vec4 outColor;

const vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
const vec3 lightColor = vec3(1.0);

void main()
{
    vec4 albedoSample = texture(gAlbedo, uvCoord);
    vec3 albedo = albedoSample.rgb;
    vec3 normal = texture(gNormal, uvCoord).rgb;
    vec3 pbr    = texture(gPBR, uvCoord).rgb;
    vec3 worldPos = texture(gPosition, uvCoord).xyz;

    if (albedoSample.a < 0.01) {
        discard;
    }

    // Safeguard against background or invalid normals
    if (length(normal) < 0.01) {
        normal = vec3(0.0, 1.0, 0.0);
    }
    normal = normalize(normal);

    float roughness = 0.1; // Default roughness
    float metallic  = 0.4; // Default metallic


    vec3 L = normalize(lightDir);
    vec3 V = normalize(ubo.cameraPosition.xyz - worldPos);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(normal, L), 0.0);
    float NdotV = max(dot(normal, V), 0.0);
    float NdotH = max(dot(normal, H), 0.0);

    // --- Diffuse ---
    vec3 diffuse = albedo;

    // --- Specular (simple GGX-lite approximation) ---
    float specPower = mix(2.0, 256.0, 1.0 - roughness);
    float spec = pow(max(dot(H, normal), 0.0), specPower);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 specular = F0 * spec;

    // --- Combine ---
    vec3 color = (diffuse * NdotL + specular) * lightColor;

    // --- Ambient ---
    color += albedo * 0.05;

    outColor = vec4(color, 1.0);
}