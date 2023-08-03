#version 430 core

in mat4 u_vp_inverse;

// Position of the fragment from vertex pass
in vec3 v_frag_pos;

uniform int u_num_cells;
uniform float u_threshold;
uniform float u_scale;
uniform vec2 u_resolution;
uniform vec3 u_camera;
uniform vec3 u_anchor_low;
uniform vec3 u_anchor_high;
uniform vec3 u_color = vec3(1.0);

layout (binding = 2) uniform sampler3D volume;

// Output
layout (location = 0) out vec4 f_color;


// float sample_density(vec3 point)
vec3 sample_density(vec3 point)
{
	vec3 offset = fract(point * u_scale);
	ivec3 cell = ivec3(floor(point * u_scale));

	cell.x %= u_num_cells;
	cell.y %= u_num_cells;
	cell.z %= u_num_cells;

	point = cell + offset;

	float min_dist = 1.0;

	for (int z = -1; z <= 1; ++z)
		for (int y = -1; y <= 1; ++y)
			for (int x = -1; x <= 1; ++x)
			{
				// ivec3 neighbor = ivec3(mod(cell + ivec3(z, y, x) + ivec3(u_num_cells), u_num_cells));
				ivec3 neighbor = ivec3(cell + ivec3(z, y, x));

				if (neighbor.x < 0)
					neighbor.x = u_num_cells - 1;
				else if (neighbor.x >= u_num_cells)
					neighbor.x = 0;

				if (neighbor.y < 0)
					neighbor.y = u_num_cells - 1;
				else if (neighbor.y >= u_num_cells)
					neighbor.y = 0;

				if (neighbor.z < 0)
					neighbor.z = u_num_cells - 1;
				else if (neighbor.z >= u_num_cells)
					neighbor.z = 0;

				// Sampling coordinates must be reversed to account for different structure in volume than generated points.
				vec3 point_in_neighbor = vec3(texelFetch(volume, ivec3(neighbor.z, neighbor.y, neighbor.x), 0)) * u_num_cells;
				float dist = length(point - point_in_neighbor);
				min_dist = min(min_dist, dist);
			}

	return vec3(point / u_num_cells);
	// return vec3(cell) / u_num_cells;
	// return 1 - min_dist;
}

vec2 get_intersections(vec3 ro, vec3 rd_inv, vec3 anchor_low, vec3 anchor_high)
{
	vec3 tmin = (anchor_low - ro) * rd_inv;
	vec3 tmax = (anchor_high - ro) * rd_inv;

	vec3 t1 = min(tmin, tmax);
	vec3 t2 = max(tmin, tmax);

	float near = max(max(t1.x, t1.y), t1.z);
	float far = min(min(t2.x, t2.y), t2.z);

	return vec2(near, far);
}

void main ()
{
	vec2 uv = 2.0 * (gl_FragCoord.xy / u_resolution) - vec2(1.0);
	float depth = gl_DepthRange.far - gl_DepthRange.near;
	float z = (2.0 * gl_FragCoord.z - depth) / (depth);

	vec4 clip = vec4(uv, z, 1.0) / gl_FragCoord.w;
	vec3 target = (u_vp_inverse * clip).xyz;

	vec3 dir = normalize(target - u_camera);

	vec2 hits = get_intersections(u_camera, 1 / dir, u_anchor_low, u_anchor_high);
	float near = hits.x;
	float far = hits.y;

	float step_size = 0.01;
	float transmittance = 0.0;

	// for (float p = near; p < far; p += step_size)
	// {
	// 	vec3 point = u_camera + dir * p;
	// 	float density = sample_density(point) * step_size;
	// 	transmittance += density;
	// }

	// if (transmittance < u_threshold)
	// 	transmittance = 0;

	// transmittance = 1 - exp(-transmittance);
	// f_color = vec4(u_color, transmittance);

	// transmittance = sample_density(u_camera + (dir * near) - u_anchor_low);
	// f_color = vec4(vec3(transmittance), 1.0);

	vec3 caca = sample_density(u_camera + (dir * near) - u_anchor_low);
	f_color = vec4(caca, 1.0);
}
