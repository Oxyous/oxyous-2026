#version 450

layout (set = 0, binding = 0) uniform sampler2D gAlbedo;
layout (set = 0, binding = 1) uniform sampler2D gNormal;
layout (set = 0, binding = 2) uniform sampler2D gPBR;
layout (set = 0, binding = 3) uniform sampler2D gPosition;
layout (set = 0, binding = 4) uniform sampler2D gDepth;
layout (set = 0, binding = 5) uniform samplerCube gEnvironment;

layout(set = 0, binding = 6) uniform PostProcessUBO {
    mat4 projection;
    mat4 view;
    mat4 invView;
    vec4 cameraPosition;
} ubo;

layout(set = 1, binding = 0) uniform sampler2DArray shadowMap;

layout(set = 1, binding = 1) uniform ShadowUBO {
    mat4 lightViewProj[4];
    vec4 cascadeSplits;
} csm;

layout (location = 0) in vec2 uvCoord;
layout (location = 0) out vec4 outColor;

const vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
const vec3 lightColor = vec3(1.0);


int getCascadeIndex(float viewDepth)
{
    int index = 0;

    for (int i = 0; i < 3; i++)
    {
        if (viewDepth > csm.cascadeSplits[i])
            index = i + 1;
    }

    return index;
}

float sampleShadow(vec3 worldPos, int cascadeIndex, vec3 normal)
{
    vec4 lightSpace =
        csm.lightViewProj[cascadeIndex] *
        vec4(worldPos, 1.0);

    vec3 proj = lightSpace.xyz / lightSpace.w;

    // NDC → texture space
    proj.xy = proj.xy * 0.5 + 0.5;

    // Outside shadow map
    if (proj.x < 0.0 || proj.x > 1.0 ||
        proj.y < 0.0 || proj.y > 1.0)
        return 1.0;

    float currentDepth = proj.z;

    // Bias (depends on slope)
    float bias = max(0.001 * (1.0 - dot(normal, -lightDir)), 0.0005);

    float shadowDepth =
        texture(shadowMap, vec3(proj.xy, cascadeIndex)).r;

    return currentDepth - bias > shadowDepth ? 0.0 : 1.0;
}

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

    float roughness = 0.9; // Default roughness
    float metallic  = 0.9; // Default metallic


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
    color +=  texture(gEnvironment, H).rgb * 0.1;

    /**/
    
   float depth = texture(gDepth, uvCoord).r;

    int cascadeIndex = getCascadeIndex(abs(depth));

    float shadow = sampleShadow(worldPos, cascadeIndex, normal);

    // Reconstruct clip space position
    vec4 clip = vec4(uvCoord * 2.0 - 1.0, 1.0, 1.0);

    // Convert to view space
    vec4 view = inverse(ubo.projection) * clip;
    view /= view.w;

    // Convert to world direction (ignore translation)
    vec3 dir = normalize((ubo.invView * vec4(view.xyz, 0.0)).xyz);

    if (texture(gDepth, uvCoord).r >= 1.0) {
        outColor = texture(gEnvironment, dir);
    }else {
        outColor = vec4(color, 1.0) * shadow;
    }
}