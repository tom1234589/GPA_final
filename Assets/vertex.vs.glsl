#version 410 core

layout(location = 0) in vec3 iv3vertex;
layout(location = 1) in vec2 iv2tex_coord;
layout(location = 2) in vec3 iv3normal;

uniform mat4 um4mv;
uniform mat4 um4p;
uniform int state;

out VS_OUT
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 V; // vector from vertex to eye
    vec2 texcoord;
} vertexData;

uniform vec4 lightPosition;

out vec3 world_coord;
out vec3 viewSpace_coord;

void main()
{
	world_coord = iv3vertex;
	vec4 P = um4mv * vec4(iv3vertex, 1.0);
	viewSpace_coord = P.xyz;
	vertexData.N = mat3(transpose(inverse(um4mv))) * iv3normal;

	// state 99 is ground
	// state 0 is city
	if(state == 99) {
		vec3 offset = vec3(-495 + (gl_InstanceID / 99)*10, 0.0, -495 + mod(gl_InstanceID, 99)*10);
		gl_Position = um4p * um4mv * vec4(iv3vertex + offset, 1.0);
	}
	else {
		gl_Position = um4p * um4mv * vec4(iv3vertex, 1.0);
	}

	vertexData.texcoord = iv2tex_coord;
	vertexData.L = lightPosition.xyz - P.xyz;
	vertexData.V = -P.xyz;
}