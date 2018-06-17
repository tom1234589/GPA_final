#version 410

layout(location = 0) in vec3 iv3vertex;
layout(location = 1) in vec2 iv2tex_coord;
layout(location = 2) in vec3 iv3normal;

uniform mat4 um4mv;
uniform mat4 um4p;
uniform mat4 um4shadow;
uniform vec4 lightPos;
uniform float seaDep;

out VertexData
{
	vec4 shadow_coord;
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 V; // eye space halfway vector
    vec2 texcoord;
} vertexData;

void main()
{
	vertexData.shadow_coord = um4shadow * vec4(iv3vertex, 1.0);
	vec4 P = um4mv * vec4(iv3vertex, 1.0);
	vertexData.N = mat3(transpose(inverse(um4mv))) * iv3normal;
	vertexData.L = lightPos.xyz - P.xyz;
	vertexData.V = -P.xyz;
	vertexData.texcoord = iv2tex_coord;

	vec4 position = vec4(iv3vertex, 1.0);
	position.y = -position.y + 2 * seaDep;

	gl_Position = um4p * um4mv * position;
}