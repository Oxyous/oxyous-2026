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

layout(set = 1, binding = 1) uniform ShadowPostProcessUBO {
    mat4 lightViewProj[4];
    vec4 cascadeSplits;
} csm;

layout (location = 0) in vec2 uvCoord;
layout (location = 0) out vec4 outColor;

const vec3 lightDir = normalize(vec3(0.5, 1.0, 0.5));
const vec3 lightColor = vec3(1.0);


int getCascadeIndex(float distance)
{
    int index = 0;

    for (int i = 0; i < 3; i++)
    {
        if (distance > csm.cascadeSplits[i])
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

    vec2 shadowCoord = proj.xy * 0.5 + 0.5;

    if (shadowCoord.x < 0.0 || shadowCoord.x > 1.0 ||
        shadowCoord.y < 0.0 || shadowCoord.y > 1.0 ||
        proj.z < 0.0 || proj.z > 1.0)
    {
        return 1.0;
    }

    float currentDepth = proj.z;

    // Better than a fixed bias
    float bias = max(
        0.00005 * (1.0 - dot(normal, normalize(vec3(0.5, 1.0, 0.5)))),
        0.00005
    );

    vec2 texelSize =
        1.0 / vec2(textureSize(shadowMap, 0).xy);

    float visibility = 0.0;

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float shadowDepth =
                texture(
                    shadowMap,
                    vec3(
                        shadowCoord + vec2(x, y) * texelSize,
                        cascadeIndex
                    )
                ).r;

            visibility +=
                (currentDepth - bias > shadowDepth)
                ? 0.2
                : 1.0;
        }
    }

    return visibility / 9.0;
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

    float roughness = 0.3; // Default roughness
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


    /**/
    float dist = length(ubo.cameraPosition.xyz - worldPos);
    int cascadeIndex = getCascadeIndex(dist);

    float shadowA = sampleShadow(worldPos, cascadeIndex, normal);
    float shadowB = sampleShadow(worldPos, min(cascadeIndex+1,3), normal);
    float shadow = shadowA;

    if (cascadeIndex < 3) {
        float blend = (dist - csm.cascadeSplits[cascadeIndex]) /
                      (csm.cascadeSplits[cascadeIndex + 1] - csm.cascadeSplits[cascadeIndex]);
        shadow = mix(shadowA, shadowB, blend);
    }

    // Reconstruct world direction from UV for skybox
    vec4 clip = vec4(uvCoord * 2.0 - 1.0, 0.0, 1.0);
    vec4 viewPt = inverse(ubo.projection) * clip;
    viewPt /= viewPt.w;
    vec3 worldDir = normalize((ubo.invView * vec4(viewPt.xyz, 0.0)).xyz);

    float depth = texture(gDepth, uvCoord).r;

    if (depth >= 1.0) {
        outColor = texture(gEnvironment, worldDir);
    }else {
        outColor = vec4(color * shadow, 1.0) + texture(gEnvironment, V).rgba * 0.1;
    }
}