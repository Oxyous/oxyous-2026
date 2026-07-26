#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inUV;
layout(location = 4) in vec4 inBoneWeights;
layout(location = 5) in ivec4 inBoneIndices;

out VS_OUTPUT
{
    layout (location = 0) vec2 uvCoord;
    layout (location = 1) vec3 worldPos;
    layout (location = 2) vec3 wNormal;
    layout (location = 3) vec3 tangent;
    layout (location = 4) vec3 bitangent;
    layout (location = 5) flat uint fragObjectIndex;
} vs_out;

layout(set = 0, binding = 2) uniform PerFrame
{
    mat4 view;
    mat4 proj;
} camera;

struct ObjectData
{
    mat4 model;
    uint materialIndex;
    uint boneIndex;
    uint pad1;
    uint pad2;
};

layout(set = 0, binding = 0) readonly buffer ObjectBuffer
{
    ObjectData objects[];
};

layout(set = 1, binding = 0) readonly buffer BoneBuffer
{
    mat4 bones[];
};

layout(push_constant) uniform PushConstants
{
    uint materialIndex;
    uint objectIndex;
} pc;

void main() {
    ObjectData obj = objects[nonuniformEXT(pc.objectIndex)];
    ivec4 boneIndices = inBoneIndices;
    vec4 fBoneWeights = inBoneWeights;

    // Each object has its own set of bones starting at boneIndex * 512
    uint boneOffset = obj.boneIndex * 512;

    mat4 transform = bones[boneOffset + boneIndices.x] * fBoneWeights.x;
    transform += bones[boneOffset + boneIndices.y] * fBoneWeights.y;
    transform += bones[boneOffset + boneIndices.z] * fBoneWeights.z;

    float finalWeight = 1.0f - (fBoneWeights.x + fBoneWeights.y + fBoneWeights.z);
    transform += bones[boneOffset + boneIndices.w] * finalWeight;

    vec4 blendedPosition = transform * vec4(inPosition, 1.0f);
    mat4 worldMatrix = obj.model;

    vec3 worldPos = vec3(worldMatrix * blendedPosition);
    mat3 normalMatrix = transpose(inverse(mat3(worldMatrix * transform)));
    vec3 normalW = normalize(normalMatrix * inNormal);

    // Tangent transformation
    vec3 tanW = normalize(mat3(worldMatrix * transform) * inTangent.xyz);
    tanW = normalize(tanW - normalW * dot(normalW, tanW));
    vec3 biTanW = normalize(cross(normalW, tanW) * inTangent.w);

    vs_out.uvCoord = inUV;
    vs_out.worldPos = worldPos;
    vs_out.wNormal  = normalW;
    vs_out.tangent  = tanW;
    vs_out.bitangent = biTanW;
    vs_out.fragObjectIndex = pc.objectIndex;

    gl_Position = camera.proj * camera.view * vec4(worldPos, 1.0f);
}
