#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 3) uniform sampler2D textures[];

struct MaterialData
{
	uint albedoTex;
	uint normalTex;
	uint ormTex;
	uint emissiveTex;

	float metallic;
	float roughness;
	float padding[2];
};

layout(set = 0, binding = 1) readonly buffer MaterialBuffer
{
	MaterialData materials[];
};

struct ObjectData
{
	mat4 model;
	uint materialIndex;
	uint pad0;
	uint pad1;
	uint pad2;
};

layout(set = 0, binding = 0) readonly buffer ObjectBuffer
{
	ObjectData objects[];
};

in VS_OUTPUT
{
	layout (location = 0) vec2 uvCoord;
	layout (location = 1) flat uint fragObjectIndex;
} vs_out;

void main()
{
	ObjectData obj = objects[nonuniformEXT(vs_out.fragObjectIndex)];
	MaterialData mat = materials[nonuniformEXT(obj.materialIndex)];

	vec4 albedo = texture(textures[nonuniformEXT(mat.albedoTex)], vs_out.uvCoord);

	if (albedo.a < 0.01)
		discard;
}
