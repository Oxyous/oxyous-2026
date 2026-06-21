#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout (set = 0, binding = 0) uniform sampler2D diffuse;
layout (set = 0, binding = 1) uniform sampler2D normal;
layout (set = 0, binding = 2) uniform sampler2D PBR;
layout (set = 0, binding = 3) uniform sampler2D wPos;

layout (location = 0) in vec2 uvCoord;
layout (location = 0) out vec4 outColor;

void main()
{
    vec4 d = texture(diffuse, uvCoord);
    vec4 n = texture(normal, uvCoord);
    vec4 p = texture(PBR, uvCoord);
    vec4 w = texture(wPos, uvCoord);

    outColor = d;
}