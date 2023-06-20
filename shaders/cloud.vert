#version 330 core

layout (location = 0) in vec3 position;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

// #1
out mat4 u_vp_inverse;

// #2
out vec3 v_frag_pos;

void main ()
{
	mat4 vp = u_projection * u_view;
	mat4 mvp = vp * u_model;

	u_vp_inverse = inverse(vp);
	v_frag_pos = (u_model * vec4 (position, 1.0)).xyz;

	gl_Position = mvp * vec4 (position, 1.0);
}
