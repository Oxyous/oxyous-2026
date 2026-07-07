#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// ========= BINDLESS TEXTURES =========
layout (set = 0, binding = 3) uniform sampler2D textures[];

// ========= OUTPUT =========
layout (location = 0) out vec4 diffuse;
layout (location = 1) out vec4 normal;
layout (location = 2) out vec4 PBR;
layout (location = 3) out vec4 wPos;

// ========= INPUT (MATCHES VERTEX) =========
in VS_OUTPUT
{
    layout (location = 0) vec2 uvCoord;
    layout (location = 1) vec3 worldPos;
    layout (location = 2) vec3 wNormal;
    layout (location = 3) vec3 tangent;
    layout (location = 4) vec3 bitangent;
    layout (location = 5) flat uint fragObjectIndex;
} vs_out;

// ========= MATERIAL =========
struct MaterialData
{
    uint albedoTex;
    uint normalTex;
    uint ormTex;
    uint emissiveTex;

    float metallic;
    float roughness;
    float padding[2];
};

layout(set = 0, binding = 1) readonly buffer MaterialBuffer
{
    MaterialData materials[];
};

// ========= OBJECT =========
struct ObjectData
{
    mat4 model;
    uint materialIndex;
    uint pad0;
    uint pad1;
    uint pad2;
};

layout(set = 0, binding = 0) readonly buffer ObjectBuffer
{
    ObjectData objects[];
};

layout(set = 0, binding = 2) uniform PerFrame
{
    mat4 view;
    mat4 proj;
} camera;

vec3 ExtractCameraWorldPos(mat4 viewMatrix)
{
    // For a rigid-body view matrix: cameraPos = -R^T * t
    mat3 rotation = mat3(viewMatrix);
    vec3 translation = viewMatrix[3].xyz;
    return -transpose(rotation) * translation;
}

vec2 ParallexMapping(vec2 uv, vec3 viewDir, float height, float heightScale)
{
    vec2 pOffset = viewDir.xy * (height * heightScale);
    return uv - pOffset;
}


vec2 ParallaxOcclusionMapping(
    vec2 texCoords,
    vec3 viewDirTS,
    sampler2D heightMap,
    float heightScale)
{
    const float minLayers = 8.0;
    const float maxLayers = 32.0;

    float numLayers = mix(
        maxLayers,
        minLayers,
        abs(viewDirTS.z)
    );

    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;

    float viewZ = max(abs(viewDirTS.z), 0.05);
    vec2 P = viewDirTS.xy / viewZ * heightScale;
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue =
        texture(heightMap, currentTexCoords).a;

    while (currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;

        currentDepthMapValue =
            texture(heightMap, currentTexCoords).a;

        currentLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    float afterDepth =
        currentDepthMapValue - currentLayerDepth;

    float beforeDepth =
        texture(heightMap, prevTexCoords).r -
        (currentLayerDepth - layerDepth);

    float weight =
        afterDepth / (afterDepth - beforeDepth);

    vec2 finalTexCoords =
        mix(currentTexCoords, prevTexCoords, weight);

    return finalTexCoords;
}


void main()
{
    // ========= FETCH OBJECT & MATERIAL =========
    ObjectData obj = objects[vs_out.fragObjectIndex];
    MaterialData mat = materials[obj.materialIndex];

    // ========= SAFE INDEXING (important for bindless debugging) =========
    uint albIdx = mat.albedoTex;
    uint nrmIdx = mat.normalTex;
    uint ormIdx = mat.ormTex;
    uint emiIdx = mat.emissiveTex;

    vec2 uv = vs_out.uvCoord;

    // Build TBN before parallax so view direction can be converted to tangent space.
    vec3 T = normalize(vs_out.tangent);
    vec3 B = normalize(vs_out.bitangent);
    vec3 N = normalize(vs_out.wNormal);
    mat3 TBN = mat3(T, B, N);

    vec3 cameraWorldPos = ExtractCameraWorldPos(camera.view);
    vec3 viewDir = cameraWorldPos - vs_out.worldPos;
    vec3 viewDirTS = normalize(transpose(TBN) * normalize(viewDir));

    float dist = sqrt(dot(viewDir,viewDir));

    float fade = 1.0 - smoothstep(5.0, 50.0, dist);
/*
    vec2 parallaxUV = ParallaxOcclusionMapping(
        uv,
        viewDirTS,
        textures[nonuniformEXT(nrmIdx)],
        0.05 * fade
    );*/

    // ========= TEXTURE SAMPLING =========
    vec4 albedo  = texture(textures[nonuniformEXT(albIdx)], uv);
    vec4 orm     = texture(textures[nonuniformEXT(ormIdx)], uv);
    vec4 emissive= texture(textures[nonuniformEXT(emiIdx)], uv);
    vec4 normalSample = texture(textures[nonuniformEXT(nrmIdx)], uv);

    if(albedo.a < 0.05)
        discard;

    // ========= NORMAL MAP DECODE =========
    // If the normal map is missing (nearly zero), default to the vertex normal
    vec3 nrm;
    if (length(normalSample.rgb) < 0.01) {
        nrm = vec3(0.0, 0.0, 1.0);
    } else {
        nrm = normalize(normalSample.rgb * 2.0 - 1.0);
    }
    vec3 worldNormal = normalize(TBN * nrm);

    // ========= OUTPUT =========
    diffuse = albedo;
    PBR     = orm;
    normal  = vec4(worldNormal, viewDir.z);
    wPos    = vec4(vs_out.worldPos, 1.0);
}