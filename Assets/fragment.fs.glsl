#version 410 core

uniform sampler2D ambient_tex;
uniform sampler2D diffuse_tex;
uniform sampler2D specular_tex;
uniform sampler2D terrain_tex_uniform;
uniform sampler2DShadow shadowmap;

out vec4 fragColor;

uniform mat4 um4mv;
uniform mat4 um4p;
uniform int state;

in VS_OUT
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 V; // eye space halfway vector
    vec2 texcoord;
	vec4 shadow_coord;
} vertexData;

uniform vec4 lightPosition;
uniform bool fogUniform;

in vec3 world_coord;
in vec3 viewSpace_coord;

struct MaterialInfo
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

uniform MaterialInfo material;

const vec4 fogColor = vec4(0.7, 0.8, 0.9, 0.0);

vec4 fog(vec4 c)
{
	float z = length(viewSpace_coord) / 80.0f;
	float fogFactor = 0;
	float fogDensity = 0.2;
	fogFactor = 1.0 / exp(z * fogDensity * z * fogDensity);
	fogFactor = clamp(fogFactor, 0.0, 1.0);
	return mix(fogColor, c, fogFactor);
}

void main()
{
	vec3 N = normalize(vertexData.N);
	vec3 L = normalize(vertexData.L);
	vec3 V = normalize(vertexData.V);
	vec3 H = normalize(L + V);
	vec3 amb, dif, spe, sum;
	float shadow;

	// state 99 is ground
	// state 0 is city
	if(state == 99) {
		sum = texture(terrain_tex_uniform, vertexData.texcoord).rgb * vec3(0.8);
		fragColor = vec4(sum, 0.7);
	}
	else if(state == 0) {
		amb = material.ambient.rgb * texture(diffuse_tex, vertexData.texcoord).rgb;
		dif = material.diffuse.rgb * texture(diffuse_tex, vertexData.texcoord).rgb * max(dot(L, N), 0.0);
		spe = material.specular.rgb * texture(diffuse_tex, vertexData.texcoord).rgb * pow(max(dot(N, H), 0.0), 64);
		shadow = textureProj(shadowmap, vertexData.shadow_coord);
		//if(shadow == 1) sum = amb + shadow * (dif + spe);
		//else sum = amb * 0.5 + shadow * (dif + spe);
		sum = amb + shadow * (dif + spe);
		if(fogUniform) fragColor = fog(vec4(sum, 1.0));
		else fragColor = vec4(sum, 1.0);
		//fragColor = texture(diffuse_tex, vertexData.texcoord);
	}
	else {
		amb = material.ambient.rgb * texture(ambient_tex, vertexData.texcoord).rgb;
		dif = material.diffuse.rgb * texture(diffuse_tex, vertexData.texcoord).rgb * max(dot(L, N), 0.0);
		spe = material.specular.rgb * texture(specular_tex, vertexData.texcoord).rgb * pow(max(dot(N, H), 0.0), 64);
		sum = amb + dif + spe;
		if(fogUniform) fragColor = fog(vec4(sum, 1.0));
		else fragColor = vec4(sum, 1.0);
	}
}