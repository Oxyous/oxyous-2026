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

    // ========= TEXTURE SAMPLING =========
    vec4 albedo  = texture(textures[nonuniformEXT(albIdx)], vs_out.uvCoord);
    vec4 orm     = texture(textures[nonuniformEXT(ormIdx)], vs_out.uvCoord);
    vec4 emissive= texture(textures[nonuniformEXT(emiIdx)], vs_out.uvCoord);
    vec4 normalSample = texture(textures[nonuniformEXT(nrmIdx)], vs_out.uvCoord);

    if(albedo.a < 0.01)
        discard;

    // ========= BUILD TBN =========
    vec3 T = normalize(vs_out.tangent);
    vec3 B = normalize(vs_out.bitangent);
    vec3 N = normalize(vs_out.wNormal);

    mat3 TBN = mat3(T, B, N);

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
    normal  = vec4(worldNormal, 1.0);
    wPos    = vec4(vs_out.worldPos, 1.0);
}