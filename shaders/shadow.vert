#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 tangent;
layout (location = 3) in vec2 uvCoord;

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
    uint pad0;
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

layout(push_constant) uniform PushConstants
{
    uint objectIndex;
    uint cascadeIndex;
} pc;

void main()
{
    ObjectData obj = objects[pc.objectIndex];

    mat4 worldMatrix = obj.model;
    
    vs_out.uvCoord = uvCoord;
    vs_out.fragObjectIndex = pc.objectIndex;

    gl_Position = csm.lightViewProj[pc.cascadeIndex] * worldMatrix *  vec4(position, 1.0);
}