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
	vec3 normal;
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

vec2 poissonDisk[16] = vec2[]( 
   vec2( -0.94201624, -0.39906216 ), 
   vec2( 0.94558609, -0.76890725 ), 
   vec2( -0.094184101, -0.92938870 ), 
   vec2( 0.34495938, 0.29387760 ), 
   vec2( -0.91588581, 0.45771432 ), 
   vec2( -0.81544232, -0.87912464 ), 
   vec2( -0.38277543, 0.27676845 ), 
   vec2( 0.97484398, 0.75648379 ), 
   vec2( 0.44323325, -0.97511554 ), 
   vec2( 0.53742981, -0.47373420 ), 
   vec2( -0.26496911, -0.41893023 ), 
   vec2( 0.79197514, 0.19090188 ), 
   vec2( -0.24188840, 0.99706507 ), 
   vec2( -0.81409955, 0.91437590 ), 
   vec2( 0.19984126, 0.78641367 ), 
   vec2( 0.14383161, -0.14100790 ) 
);


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
	float cosTheta = clamp(dot(N, L), 0, 1);
	float visibility=1.0;
	float bias = 0.005;

	// state 99 is ground
	// state 0 is city
	if(state == 99) {
		vec3 baseColor = texture(terrain_tex_uniform, vertexData.texcoord).rgb;
		vec3 reflectVec = reflect(normalize(world_coord - eyePosition), normalize(vertexData.normal));
		vec3 reflectColor = textureCube(tex_cubemap, reflectVec).rgb;
		
		for (int i = 0; i < 4; i++) {
			visibility -= 0.1*(1.0-texture(shadowmap, vec3(vertexData.shadow_coord.xy + poissonDisk[i]/700.0,  (vertexData.shadow_coord.z-bias)/vertexData.shadow_coord.w)));
		}
		if(visibility < 0.5) visibility = 0.5;
		sum = visibility * reflectColor * baseColor;
		fragColor = vec4(sum, 0.7);
	}
	else if(state == 0) {
		amb = material.ambient.rgb * texture(diffuse_tex, vertexData.texcoord).rgb;
		dif = material.diffuse.rgb * texture(diffuse_tex, vertexData.texcoord).rgb * max(dot(L, N), 0.0);
		spe = material.specular.rgb * texture(diffuse_tex, vertexData.texcoord).rgb * pow(max(dot(N, H), 0.0), 64);
		for (int i = 0; i < 4; i++) {
			visibility -= 0.2*(1.0-texture(shadowmap, vec3(vertexData.shadow_coord.xy + poissonDisk[i]/700.0,  (vertexData.shadow_coord.z-bias)/vertexData.shadow_coord.w)));
		}

		sum = amb + visibility * (dif + spe);
		if(fogUniform) fragColor = fog(vec4(sum, 1.0));
		else fragColor = vec4(sum, 1.0);
	}
	else {
		amb = material.ambient.rgb * texture(ambient_tex, vertexData.texcoord).rgb;
		dif = material.diffuse.rgb * texture(diffuse_tex, vertexData.texcoord).rgb * max(dot(L, N), 0.0);
		spe = material.specular.rgb * texture(specular_tex, vertexData.texcoord).rgb * pow(max(dot(N, H), 0.0), 64);
		for (int i = 0; i < 4; i++) {
			visibility -= 0.2*(1.0-texture(shadowmap, vec3(vertexData.shadow_coord.xy + poissonDisk[i]/700.0,  (vertexData.shadow_coord.z-bias)/vertexData.shadow_coord.w)));
		}

		sum = amb + visibility * (dif + spe);
		if(fogUniform) fragColor = fog(vec4(sum, 1.0));
		else fragColor = vec4(sum, 1.0);
	}
}