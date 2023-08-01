#version 330 core

// Input
// layout (binding = 0) uniform sampler3D volume;

uniform vec2 u_resolution;

// Output
layout (location = 0) out vec4 f_color;

void main ()
{
	vec2 uv = 2.0 * (gl_FragCoord.xy / u_resolution) - vec2(1.0);
	f_color = vec4(1.0, 1.0, 1.0, 1.0);
}
