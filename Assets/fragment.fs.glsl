#version 410 core
#extension GL_NV_shadow_samplers_cube : enable

uniform sampler2D ambient_tex;
uniform sampler2D diffuse_tex;
uniform sampler2D specular_tex;
uniform sampler2D terrain_tex_uniform;
uniform sampler2DShadow shadowmap;
uniform samplerCube tex_cubemap;

out vec4 fragColor;

uniform mat4 um4mv;
uniform mat4 um4p;
uniform int state;

in VS_OUT
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 V; // eye space halfway vector
	vec3 view;
    vec2 texcoord;
	vec4 shadow_coord;
} vertexData;

uniform vec4 lightPosition;
uniform vec3 eyePosition;
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
float Eta = 0.6;
float FresnelPower=5.0;
float F=((1.0-Eta)*(1.0-Eta))/((1.0+Eta)*(1.0+Eta));

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
	int half_size = 2;
	int n;

	// state 99 is ground
	// state 0 is city
	if(state == 99) {
		vec3 baseColor = texture(terrain_tex_uniform, vertexData.texcoord).rgb;
		vec3 reflectVec = reflect(normalize(world_coord - eyePosition), N);
		vec3 reflectColor = textureCube(tex_cubemap, reflectVec).rgb;
		fragColor = vec4(reflectColor * baseColor, 0.7);
	}
	else if(state == 0) {
		amb = material.ambient.rgb * texture(diffuse_tex, vertexData.texcoord).rgb;
		dif = material.diffuse.rgb * texture(diffuse_tex, vertexData.texcoord).rgb * max(dot(L, N), 0.0);
		spe = material.specular.rgb * texture(diffuse_tex, vertexData.texcoord).rgb * pow(max(dot(N, H), 0.0), 64);
		shadow = 0.0;
		n = 0;
		for(int i = -half_size; i <= half_size; i++) {
			for(int j = -half_size; j <= half_size; j++) {
				shadow += textureProj(shadowmap, vertexData.shadow_coord + vec4(i, j, 0, 0));
				n++;
			}
		}
		shadow /= n;

		sum = amb + shadow * (dif + spe);
		if(fogUniform) fragColor = fog(vec4(sum, 1.0));
		else fragColor = vec4(sum, 1.0);
	}
	else {
		amb = material.ambient.rgb * texture(ambient_tex, vertexData.texcoord).rgb;
		dif = material.diffuse.rgb * texture(diffuse_tex, vertexData.texcoord).rgb * max(dot(L, N), 0.0);
		spe = material.specular.rgb * texture(specular_tex, vertexData.texcoord).rgb * pow(max(dot(N, H), 0.0), 64);
		shadow = 0.0;
		n = 0;
		for(int i = -half_size; i <= half_size; i++) {
			for(int j = -half_size; j <= half_size; j++) {
				shadow += textureProj(shadowmap, vertexData.shadow_coord + vec4(i, j, 0, 0));
				n++;
			}
		}
		shadow /= n;

		sum = amb + shadow * (dif + spe);
		if(fogUniform) fragColor = fog(vec4(sum, 1.0));
		else fragColor = vec4(sum, 1.0);
	}
}