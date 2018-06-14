#version 420 core

layout (binding = 0) uniform sampler2D diffuse_tex;
layout (binding = 1) uniform sampler2D ambient_tex;
layout (binding = 2) uniform sampler2D specular_tex;

out vec4 fragColor;

uniform mat4 um4mv;
uniform mat4 um4p;
uniform int state;
layout (binding = 3) uniform sampler2D terrain_tex_uniform;

in VS_OUT
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 V; // eye space halfway vector
    vec2 texcoord;
} vertexData;

uniform vec4 lightPosition;

struct MaterialInfo
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

uniform MaterialInfo material;

void main()
{
	vec3 N = normalize(vertexData.N);
	vec3 L = normalize(vertexData.L);
	vec3 V = normalize(vertexData.V);
	vec3 H = normalize(L + V);
	vec3 amb, dif, spe, sum;

	if(state == 0) {
		fragColor = texture(terrain_tex_uniform, vertexData.texcoord);
	}
	else if(state == 1) {
		amb = vec3(0.2, 0.2, 0.2) * texture(diffuse_tex, vertexData.texcoord).rgb;
		dif = vec3(0.8, 0.8, 0.8) * texture(diffuse_tex, vertexData.texcoord).rgb * max(dot(L, N), 0.0);
		spe = vec3(1.0, 1.0, 1.0) * texture(diffuse_tex, vertexData.texcoord).rgb * pow(max(dot(N, H), 0.0), 64);
		sum = amb + dif + spe;
		fragColor = vec4(sum, 1.0);
	}
	else {
		fragColor = texture(diffuse_tex, vertexData.texcoord);
	}
}