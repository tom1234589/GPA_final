#version 420

layout (binding = 0) uniform sampler2D diffuse_tex;
layout (binding = 1) uniform sampler2D ambient_tex;
layout (binding = 2) uniform sampler2D specular_tex;

out vec4 fragColor;

uniform mat4 um4mv;
uniform mat4 um4p;
uniform int state;
layout (binding = 3) uniform sampler2D terrain_tex_uniform;

in VertexData
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 H; // eye space halfway vector
    vec2 texcoord;
} vertexData;

struct MaterialInfo
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

uniform MaterialInfo material;

void main()
{
	if(state == 0) {
		fragColor = texture(terrain_tex_uniform, vertexData.texcoord);
	}
	else {
		vec3 amb, dif, spe;
		if(texture(ambient_tex, vertexData.texcoord).rgb == vec3(0)) amb = material.ambient.rgb;
		else amb = material.ambient.rgb * texture(ambient_tex, vertexData.texcoord).rgb;
		if(texture(diffuse_tex, vertexData.texcoord).rgb == vec3(0)) dif = material.diffuse.rgb;
		else dif = material.diffuse.rgb * texture(diffuse_tex, vertexData.texcoord).rgb;
		spe = material.specular.rgb * texture(specular_tex, vertexData.texcoord).rgb;
		vec3 texColor = amb + dif + spe;
		fragColor = vec4(texColor, 1.0);
	}
}