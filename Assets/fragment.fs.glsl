#version 410 core

uniform sampler2D ambient_tex;
uniform sampler2D diffuse_tex;
uniform sampler2D specular_tex;
uniform sampler2D terrain_tex_uniform;

out vec4 fragColor;

uniform mat4 um4mv0;
uniform mat4 um4mv90;
uniform mat4 um4mv180;
uniform mat4 um4mv270;
uniform mat4 um4p;
uniform int state;


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
		sum = texture(terrain_tex_uniform, vertexData.texcoord).rgb * vec3(0.8);
		fragColor = vec4(sum, 1.0);
	}
	else if(state == 1) {
		amb = vec3(0.2, 0.2, 0.2) * texture(diffuse_tex, vertexData.texcoord).rgb;
		dif = vec3(0.8, 0.8, 0.8) * texture(diffuse_tex, vertexData.texcoord).rgb * max(dot(L, N), 0.0);
		spe = vec3(1.0, 1.0, 1.0) * texture(diffuse_tex, vertexData.texcoord).rgb * pow(max(dot(N, H), 0.0), 64);
		sum = amb + dif + spe;
		fragColor = vec4(sum, 1.0);
	}
	else {
		amb = material.ambient.rgb * texture(ambient_tex, vertexData.texcoord).rgb;
		dif = material.diffuse.rgb * texture(diffuse_tex, vertexData.texcoord).rgb * max(dot(L, N), 0.0);
		spe = material.specular.rgb * texture(specular_tex, vertexData.texcoord).rgb * pow(max(dot(N, H), 0.0), 64);
		sum = amb + dif + spe;
		fragColor = vec4(sum, 1.0);
	}
}