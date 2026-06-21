#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout (set = 0, binding = 1)  uniform sampler2D diffuseTexture;
layout (set = 0, binding = 2)  uniform sampler2D normalTexture;
layout (set = 0, binding = 3)  uniform sampler2D PBRTexture;

layout (location = 0) out vec4 diffuse;
layout (location = 1) out vec4 normal;
layout (location = 2) out vec4 PBR;
layout (location = 3) out vec4 wPos;

in VS_OUTPUT
{
    layout (location = 0) vec2 uvCoord;
    layout (location = 1) vec3 worldPos;
    layout (location = 2) vec3 normal; 
    layout (location = 3) mat3 tanTrans;
}vs_out;

void main()
{
	diffuse = texture(diffuseTexture, vs_out.uvCoord, 0);
	//diffuse = vec4(vs_out.uvCoord,0.0,1.0);
	//if(diffuse.a < 0.5f)
	//	discard;asd

	PBR = texture(PBRTexture, vs_out.uvCoord, 0);
	vec3 nm = (texture(normalTexture, vs_out.uvCoord) * 2.0 - 1.0).xyz;
	normal = vec4(nm * mat3(vs_out.tanTrans), gl_FragCoord.z);
    //normal = vec4(vs_out.normal,1.0) * 0.5 + 0.5;
	wPos = vec4(vs_out.worldPos, 1.0f);
}
