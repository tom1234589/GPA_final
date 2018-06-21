#version 410

layout(location = 0) out vec4 fragColor;

uniform mat4 um4mv;
uniform mat4 um4p;

in VertexData
{
	vec4 shadow_coord;
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 V; // eye space halfway vector
    vec2 texcoord;
} vertexData;

uniform sampler2D tex;
uniform sampler2DShadow shadowmap;
uniform vec4 Ka;
uniform vec4 Kd;
uniform vec4 Ks;

void main()
{
	vec3 N = normalize(vertexData.N);
	vec3 L = normalize(vertexData.L);
	vec3 V = normalize(vertexData.V);
	vec3 H = normalize(L + V);
	float theta = max(dot(N, L), 0.0);
	float phi =  max(dot(N, H), 0.0);
	vec3 texColor = texture(tex, vertexData.texcoord).rgb;
	if(texColor == vec3(0))
	{
		discard;
	}
	vec3 ambient = texColor * Ka.rgb;
	vec3 diffuse = texColor * Kd.rgb * theta;
	vec3 specular = vec3(1,1,1) * Ks.rgb * pow(phi, 64);
	float shadow = textureProj(shadowmap, vertexData.shadow_coord);
	
	vec3 lighting;
	
	if(shadow == 1){
		lighting = (ambient + (textureProj(shadowmap, vertexData.shadow_coord)) * (diffuse + specular));
	}
	else{
		lighting = (ambient * 0.5 + (textureProj(shadowmap, vertexData.shadow_coord)) * (diffuse + specular));
	}
	fragColor = vec4(lighting, 1.0);
}