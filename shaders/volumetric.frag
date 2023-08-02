#version 330 core

in mat4 u_vp_inverse;

// Position of the fragment from vertex pass
in vec3 v_frag_pos;

uniform sampler3D volume;

uniform vec2 u_resolution;
uniform vec3 u_camera;
uniform vec3 u_anchor_low = vec3(-0.5, -0.5, -0.5);
uniform vec3 u_anchor_high = vec3(0.5, 0.5, 0.5);
uniform vec4 u_color = vec4(0.0, 0.8, 0.3, 1.0);
uniform vec4 u_fog_color = vec4(0.608, 0.671, 0.733, 0.0);

uniform int steps = 10;
uniform float cloud_scale = 50.0;
uniform float density_multiplier = 5.0;
uniform float density_threshold = 0.9;
uniform vec3 cloud_offset = vec3(0.0);

// Output
layout (location = 0) out vec4 f_color;


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

float sample_density(vec3 position)
{
	vec3 uvw = position * cloud_scale * 0.001 + cloud_offset * 0.01;
	float data = texture3D(volume, uvw).r;
	float density = max(0, data - density_threshold) * density_multiplier;
	return density;
}

float get_final_color(vec3 ray_dir, float near, float far)
{
	int num_steps = 5;
	float step_size = (far - near) / num_steps;
	float traveled = near;

	float total_density = 0.0;

	while (traveled < far)
	{
		vec3 point = u_camera + ray_dir * (traveled);
		total_density += sample_density(point) * step_size;
		traveled += step_size;
	}

	float transmittance = exp(-total_density);
	return transmittance;
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
	vec3 near = u_camera + dir * hits.x;
	vec3 far = u_camera + dir * hits.y;

	float cloud_color = get_final_color(dir, hits.x, hits.y);

	cloud_color = clamp(cloud_color, 0.0, 1.0);

	f_color = vec4(1.0, 1.0, 1.0, cloud_color);
	// f_color = u_color * cloud_color;
}
