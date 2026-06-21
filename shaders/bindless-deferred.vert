#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 tangent;
layout (location = 3) in vec2 uvCoord;

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
    uint pad0;
    uint pad1;
};

layout(set = 0, binding = 0) readonly buffer ObjectBuffer
{
    ObjectData objects[];
};

layout(push_constant) uniform PushConstants
{
    uint objectIndex;
} pc;

void main()
{
    ObjectData obj = objects[pc.objectIndex];

    mat4 worldMatrix = obj.model;
    mat4 mvp = camera.proj * camera.view * worldMatrix;

    mat3 normalMatrix = transpose(inverse(mat3(worldMatrix)));

    vec3 worldPos = vec3(worldMatrix * vec4(position, 1.0));

    vec3 normalW = normalize(normalMatrix * normal);
    vec3 tanW    = normalize(normalMatrix * tangent.xyz);
    vec3 biTanW  = normalize(cross(normalW, tanW) * tangent.w);

    vs_out.uvCoord = uvCoord;
    vs_out.worldPos = worldPos;
    vs_out.wNormal  = normalW;
    vs_out.tangent  = tanW;
    vs_out.bitangent = biTanW;

    vs_out.fragObjectIndex = pc.objectIndex;

    gl_Position = mvp * vec4(position, 1.0);
}