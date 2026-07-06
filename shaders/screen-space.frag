#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// ========= BINDLESS TEXTURES =========
layout (set = 0, binding = 3) uniform sampler2D textures[];

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

layout(push_constant) uniform PushConstants
{
    mat4 model;
    uint textureIndex;
    uint objectIndex;
} pc;

in VS_OUTPUT
{
    layout (location = 0) vec2 uvCoord;
    layout (location = 1) flat uint fragObjectIndex;
}vs_out;

layout (location = 0) out vec4 fragColor;

void main()
{
    ObjectData obj = objects[nonuniformEXT(vs_out.fragObjectIndex)];
    fragColor = texture(textures[nonuniformEXT(obj.textureId)], vs_out.uvCoord);

    if(fragColor.a < 0.05)
      discard;
}