#version 330 core

layout (location = 0) in vec3 position;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main ()
{
    mat4 mvp = u_projection * u_view * u_model;
    gl_Position = mvp * vec4 (position, 1.0f);
}
