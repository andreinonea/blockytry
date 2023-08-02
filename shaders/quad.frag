#version 430 core

// Input
// layout (binding = 0) uniform sampler3D volume;

uniform vec2 u_resolution;
uniform int u_num_cells;
uniform float u_slice = 0.0;

layout (binding = 2) uniform sampler3D volume;

in vec2 v_tex_coords;

// Output
layout (location = 0) out vec4 f_color;

void main ()
{
	ivec3 cell = ivec3(floor(vec3(v_tex_coords, u_slice) * u_num_cells));
	vec3 point_in_cell = fract(vec3(v_tex_coords, u_slice) * u_num_cells);

	float min_dist = 1.0;

	for (int z = -1; z <= 1; ++z)
		for (int y = -1; y <= 1; ++y)
			for (int x = -1; x <= 1; ++x)
			{
				ivec3 neighbor = (cell + ivec3(x, y, z) + ivec3(u_num_cells)) % u_num_cells;
				vec3 point_in_neighbor = vec3(texelFetch(volume, neighbor, 0));
				float dist = length(cell + point_in_cell - point_in_neighbor);
				min_dist = min(min_dist, dist);
			}

	if (point_in_cell.x < 0.005 || point_in_cell.y < 0.005)
		f_color = vec4(1.0, 0.0, 0.0, 1.0);
	else
		f_color = vec4(vec3(min_dist), 1.0);
}
