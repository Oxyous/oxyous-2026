#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uvCoord;

struct ObjectData
{
    mat4 model;
    uint textureId;
    uint pad0;
    uint pad1;
    uint pad2;
};

layout(set = 0, binding = 0) readonly buffer ObjectBuffer
{
    ObjectData objects[];
};

layout (set = 0, binding = 1) uniform perFrame
{
    mat4 orthoProjection;
    vec2 screenSize;
};

out VS_OUTPUT
{
    layout (location = 0) vec2 uvCoord;
    layout (location = 1) flat uint fragObjectIndex;
}vs_out;

layout(push_constant) uniform PushConstants
{
    uint textureIndex;
    uint objectIndex;
} pc;


void main()
{
    ObjectData obj = objects[pc.objectIndex];

    vs_out.uvCoord = uvCoord;
    vs_out.fragObjectIndex = pc.objectIndex;

    vec4 world = obj.model * vec4(position, 0.0, 1.0);
    gl_Position = orthoProjection * world;
}
