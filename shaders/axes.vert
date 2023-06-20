#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

uniform mat4 u_vp;

out vec3 v_color;

void main ()
{
    gl_Position = u_vp * vec4 (position, 1.0f);
    v_color = color;
}
