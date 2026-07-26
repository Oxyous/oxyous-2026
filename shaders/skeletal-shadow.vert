#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 tangent;
layout(location = 3) in vec2 uvCoord;
layout(location = 4) in vec4 fBoneWeights;
layout(location = 5) in ivec4 boneIndices;

out VS_OUTPUT
{
    layout (location = 0) vec2 uvCoord;
    layout (location = 1) flat uint fragObjectIndex;
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


layout(set = 1, binding = 0) uniform ShadowCaptureUBO
{
    mat4 lightViewProj[4];
    vec4 cascadeSplits;
} csm;


layout(set = 0, binding = 0) readonly buffer ObjectBuffer
{
    ObjectData objects[];
};

layout(set = 2, binding = 0) readonly buffer BoneBuffer
{
    mat4 bones[];
};

layout(push_constant) uniform PushConstants
{
    uint objectIndex;
    uint cascadeIndex;
} pc;

void main()
{
    ObjectData obj = objects[pc.objectIndex];

    // Each object has its own set of bones starting at boneIndex * 512
    uint boneOffset = obj.boneIndex * 512;
    
    mat4 transform = bones[boneOffset + boneIndices.x] * fBoneWeights.x;
    transform += bones[boneOffset + boneIndices.y] * fBoneWeights.y;
    transform += bones[boneOffset + boneIndices.z] * fBoneWeights.z;

    float finalWeight = 1.0f - (fBoneWeights.x + fBoneWeights.y + fBoneWeights.z);
    transform += bones[boneOffset + boneIndices.w] * finalWeight;

    vec4 blendedPosition = transform * vec4(position, 1.0f);

    mat4 worldMatrix = obj.model;
    
    vs_out.uvCoord = uvCoord;
    vs_out.fragObjectIndex = pc.objectIndex;

    gl_Position = csm.lightViewProj[pc.cascadeIndex] * worldMatrix * blendedPosition;
}
