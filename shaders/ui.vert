#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;

layout(location = 2) in vec2 spritePos;
layout(location = 3) in vec2 spriteSize;
layout(location = 4) in vec2 spriteUVOffset;
layout(location = 5) in vec2 spriteUVScale;

layout(location = 0) out vec2 fragUV;

layout(push_constant) uniform PushConstants
{
    mat4 projection;
};

void main()
{
    vec2 worldPos =
        spritePos +
        inPos * spriteSize;

    fragUV = mix(spriteUVOffset.xy, spriteUVScale.xy, inUV);

    gl_Position =
        projection *
        vec4(worldPos, 0.0, 1.0);
}