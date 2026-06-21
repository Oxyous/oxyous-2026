#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 position;
layout (location = 1) out vec2 uvCoord;

const vec2 ssVertices[4] = vec2[](
    vec2(1.0, -1.0),
    vec2(-1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, 1.0)
);

const vec2 uvCoords[4] = vec2[](
    vec2(1.0, 0.0),
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

void main()
{
    position = vec4(ssVertices[gl_VertexIndex], 0.0f, 1.0f);
    uvCoord = uvCoords[gl_VertexIndex];
    gl_Position = position;
}