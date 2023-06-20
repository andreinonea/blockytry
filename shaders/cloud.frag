#version 330 core

in mat4 u_vp_inverse;

// Position of the fragment from vertex pass
in vec3 v_frag_pos;

uniform vec2 u_resolution;
uniform vec3 u_camera;
uniform vec3 u_anchor_low = vec3(-0.5, -0.5, -0.5);
uniform vec3 u_anchor_high = vec3(0.5, 0.5, 0.5);
uniform vec4 u_color = vec4(0.0, 0.8, 0.3, 1.0);
uniform vec4 u_fog_color = vec4(0.608, 0.671, 0.733, 0.0);

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

void main ()
{
	vec2 uv = 2.0 * (gl_FragCoord.xy / u_resolution) - vec2(1.0);
	float depth = gl_DepthRange.far - gl_DepthRange.near;
	float z = (2.0 * gl_FragCoord.z - depth) / (depth);

	vec4 clip = vec4(uv, z, 1.0) / gl_FragCoord.w;
	vec3 target = (u_vp_inverse * clip).xyz;

	vec3 dir = normalize(target - u_camera);
	// vec3 dir_v = normalize(v_frag_pos - u_camera);

	vec2 hits = get_intersections(u_camera, 1 / dir, u_anchor_low, u_anchor_high);

	float dist_in_cube = clamp(hits.y - hits.x, 0.0, 1.0);

	float visibility = clamp(exp(-dist_in_cube), 0.0, 1.0);

	f_color = mix(u_color, u_fog_color, visibility);
	// f_color = u_color * (1-visibility);

	// if (dir == dir_v)
	// {
	// 	f_color = vec4(1.0, 1.0, 1.0, 1.0);
	// }
	// else
	// {
	// 	f_color = vec4(0.0, 0.0, 0.0, 1.0);
	// }
}
