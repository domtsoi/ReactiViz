#version 450 core
layout(location = 0) out vec4 color;

in vec2 vertex_tex;
in vec3 vertex_pos;

uniform sampler2D depth_tex;
uniform sampler2D color_tex;
uniform sampler2D static_tex;

const int num_joints = 25;
const int max_bodies = 6;
const float max_timestamp = 10.f;

uniform vec4 bodies[max_bodies * num_joints];
uniform int time_stamps[max_bodies];
uniform int num_bodies;
uniform float time;
uniform int music_influence;
//PLAY WITH MIN THRESHHOLD VALUES TO GET BETTER BLEND (If max thresh == minthresh background == body static lvl)
const float min_thresh = 0.01;
const float max_thresh = 0.05;
const float xy_epsilon = 0.08;
const float z_epsilon = 0.01;
const float min_z = 0.15;
// ================ below is copied code ========================== //
// https://www.shadertoy.com/view/ltB3zD
const float phi = 1.6180339887 * 00000.1;
const float pi = 3.14159265358 * 00000.1;
const float sq2 = 1.4142135623 * 10000.0;
const float tiny = 0.00001;
float gold_noise(vec2 coord, float seed)
{
	return fract(tan(distance(coord * (seed + phi), vec2(phi, pi)))*sq2);
}
// https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}
// ================ above is copied code ========================== //

float get_threshold(float time_stamp)
{
	float thresh_diff = max_thresh - min_thresh;
	float percent_done = time_stamp / max_timestamp;
	return min_thresh + (thresh_diff * (1.f - percent_done));
}

void main()
{

	vec4 back_color = vec4(0, 0, 0, 1);
	// if (music_influence == THEME1) back_color = vec4(THEME1_R, ..., 1);
	vec3 pos = vec3(vertex_tex, texture(depth_tex, vertex_tex).r);
	vec2 npos = vertex_tex + vec2(time);
	float r = rand(npos);
	for (int i = 0; i < num_bodies; ++i)
	{
		for (int j = 0; j < num_joints; ++j)
		{
			float xy_diff = length(pos.xy - bodies[(i * num_joints) + j].xy);
			float z_diff = abs(pos.z - bodies[(i * num_joints) + j].z);
			if (pos.z > min_z && xy_diff < xy_epsilon && z_diff < z_epsilon)
			{
				color = vec4(gold_noise(npos, phi),
							 gold_noise(npos, pi),
							 gold_noise(npos, sq2),
													   1);

				if (r > get_threshold(abs(time_stamps[i]))) color = back_color;
				return;
			}
		}
	}

	
	color = vec4(gold_noise(npos, phi),
				 gold_noise(npos, pi),
				 gold_noise(npos, sq2),
										   1);

	if (r > min_thresh) color = back_color;
}