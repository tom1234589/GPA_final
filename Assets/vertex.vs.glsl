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

void main()
{
	vec4 P = um4mv * um4p * vec4(iv3vertex, 1.0);
	vertexData.N = mat3(transpose(inverse(um4mv))) * iv3normal;
	vertexData.L = lightPosition.xyz;
	vertexData.V = -P.xyz;
	if(state == 0) {
		vec3 offset = vec3(-495 + (gl_InstanceID / 99)*10, 0.0, -495 + mod(gl_InstanceID, 99)*10);
		gl_Position = um4p * um4mv * vec4(iv3vertex + offset, 1.0);
		vertexData.texcoord = iv2tex_coord;
	}
	else {
		gl_Position = um4p * um4mv * vec4(iv3vertex, 1.0);
		vertexData.texcoord = iv2tex_coord;
	}
}