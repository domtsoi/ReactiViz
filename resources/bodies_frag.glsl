#version 450 core
layout(location = 0) out vec4 color;

in vec2 vertex_tex;
in vec3 vertex_pos;

uniform sampler2D body1_tex;
uniform sampler2D body2_tex;
uniform sampler2D body3_tex;
uniform sampler2D body4_tex;
uniform sampler2D body5_tex;
uniform sampler2D body6_tex;

uniform int num_bodies;
uniform int time_stamps[6];

const float epsilon = 0.1;
const float max_timestamp = 10.f;

void main()
{
	// TODO: change colors from red/green to variations of noise
	if (num_bodies > 0 && texture(body1_tex, vertex_tex).r > epsilon)
	{
		color = vec4(1, 0, 0, 1.f); // - (float(time_stamps[0])/max_timestamp)); 
	} 
	else if (num_bodies > 1 && texture(body2_tex, vertex_tex).r > epsilon)
	{
		color = vec4(1, 0, 0, 1.f); //- (float(time_stamps[1])/max_timestamp)); 
	}
	else if (num_bodies > 2 && texture(body3_tex, vertex_tex).r > epsilon)
	{
		color = vec4(1, 0, 0, 1.f); // - (float(time_stamps[2])/max_timestamp)); 
	}
	else if (num_bodies > 3 && texture(body4_tex, vertex_tex).r > epsilon)
	{
		color = vec4(1, 0, 0, 1.f); // - (float(time_stamps[3])/max_timestamp)); 
	}
	else if (num_bodies > 4 && texture(body5_tex, vertex_tex).r > epsilon)
	{
		color = vec4(1, 0, 0, 1.f); // - (float(time_stamps[4])/max_timestamp)); 
	}
	else if (num_bodies > 5 && texture(body6_tex, vertex_tex).r > epsilon)
	{
		color = vec4(1, 0, 0, 1.f); // - (float(time_stamps[5])/max_timestamp)); 
	}
	else
	{
		color = vec4(0, 1, 0, 1);
	}
}