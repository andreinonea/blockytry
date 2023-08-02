#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 tex_coords;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

out vec2 v_tex_coords;

void main ()
{
    v_tex_coords = tex_coords;
    mat4 mvp = u_projection * u_view * u_model;
    gl_Position = mvp * vec4 (position, 0.0, 1.0);
}
