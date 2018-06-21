#version 410 core

layout(location = 0) in vec3 iv3vertex;
layout(location = 1) in vec2 iv2tex_coord;
layout(location = 2) in vec3 iv3normal;

uniform mat4 um4mv;
uniform mat4 um4p;
uniform mat4 um4shadow;
uniform int state;

out VS_OUT
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 V; // vector from vertex to eye
    vec2 texcoord;
	vec4 shadow_coord;
} vertexData;

uniform vec4 lightPosition;
uniform int time;

out vec3 world_coord;
out vec3 viewSpace_coord;

int n = 3;
int m = 2;
float thetas[2] = float[](0.38, 1.42);
float amplitudes[6] = float[](0.02, 0.02, 0.03, 0.05, 0.02, 0.06);  
float omegas[3] = float[](3.27,3.31,3.42);
float waveNums[3] = float[](1.091,1.118,1.1935);

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
		vec3 newPos = iv3vertex + offset;
		float x = newPos.x;
		float y = newPos.y;
		float z = newPos.z;
		float t = time / 100.0;
		for(int i = 0; i < n; i++)  
		{  
			for(int j = 0; j < m; j++)  
			{
				x -= cos(thetas[j]) * amplitudes[i*3+j] * sin(waveNums[i] * (x * cos(thetas[j]) + z * sin(thetas[j])) - omegas[i] * t);
				y += amplitudes[i*3+j] * cos(waveNums[i] * ( x * cos(thetas[j]) + z * sin(thetas[j])) - omegas[i] * t) * 0.5;
				z -= sin(thetas[j]) * amplitudes[i*3+j] * sin(waveNums[i] * (x * cos(thetas[j]) + z * sin(thetas[j])) - omegas[i] * t);
			}
		}
		newPos = vec3(x, y, z);
		gl_Position = um4p * um4mv * vec4(newPos, 1.0);
	}
	else {
		gl_Position = um4p * um4mv * vec4(iv3vertex, 1.0);
	}

	vertexData.texcoord = iv2tex_coord;
	vertexData.L = lightPosition.xyz - P.xyz;
	vertexData.V = -P.xyz;
	vertexData.shadow_coord = um4shadow * vec4(iv3vertex, 1.0);
}