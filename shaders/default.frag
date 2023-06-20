#version 330 core

// Position of the fragment from vertex pass
in vec3 v_frag_pos;

// Diffuse color
uniform vec4 u_some_color;

// Eye position
uniform vec3 u_camera_pos;

// Fog
uniform vec4 u_fog_color = vec4(0.608, 0.671, 0.733, 1.0);
uniform float u_fog_absorption = 0.08;
uniform float u_fog_gradient = 2.0;

// Output
layout (location = 0) out vec4 f_color;

void main ()
{
	// Fog
	float distance_to_camera = length(u_camera_pos - v_frag_pos);
	float fog_factor = exp(-pow(distance_to_camera * u_fog_absorption, u_fog_gradient));
	fog_factor = clamp(fog_factor, 0.0, 1.0);

	f_color = mix(u_fog_color, u_some_color, fog_factor);
}
