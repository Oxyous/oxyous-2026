#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 tangnet;
layout (location = 3) in vec2 uvCoord;

layout (set = 0, binding = 0) uniform perObject
{
    mat4 worldMatrix;
};

layout (set = 1, binding = 0) uniform perFrame
{
    mat4 viewProjection;
    mat4 viewMatrix;
};

out VS_OUTPUT
{
    layout (location = 0) vec2 uvCoord;
    layout (location = 1) vec3 worldPos;
    layout (location = 2) vec3 wNormal;
    layout (location = 3) mat3 tanTrans; 
}vs_out;

void main() {
    mat4 mvp = viewProjection * viewMatrix * worldMatrix;

    mat4 worldView = viewMatrix * worldMatrix;

    mat3 normalMatrix = mat3(inverse(worldMatrix));

    vec3 tanW = normalize(tangnet.xyz * normalMatrix);

    vec3 normalW = normalize(normal.xyz * normalMatrix);

    vec3 biTanW = normalize(cross(tanW, normalW) * tangnet.w);

    vs_out.uvCoord = vec2(uvCoord.x, uvCoord.y);

    vs_out.tanTrans = transpose(mat3(tanW, biTanW, normalW));
    
    vs_out.worldPos = vec3(worldMatrix * vec4(position.xyz, 1.0f));

    vs_out.wNormal = normal;

    gl_Position = mvp * vec4(position.xyz, 1.0f);
}