#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

out vec3 v_color;

void main ()
{
    mat4 mvp = u_projection * u_view;
    gl_Position = mvp * vec4 (position, 1.0f);
    v_color = color;
}
