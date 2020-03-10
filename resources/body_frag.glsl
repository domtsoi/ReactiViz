#version 450 core
layout(location = 0) out vec4 color;

in vec2 vertex_tex;
in vec3 vertex_pos;

uniform sampler2D depth_tex;
uniform sampler2D color_tex;
uniform sampler2D static_tex;
uniform sampler2D tv_tex;

const int num_joints = 25;
const int max_bodies = 6;
const float max_timestamp = 10.f;

uniform vec4 bodies[max_bodies * num_joints];
uniform int time_stamps[max_bodies];
uniform int num_bodies;
uniform float time;
uniform float music_influence;
uniform float kinect_depth;
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

vec3 wrongcolor(float curD, float dLimit)
{
	float range1 = dLimit *.2;
	float range2 = dLimit *.4;
	float range3 = dLimit *.6;
	float range4 = dLimit *.8;
	float range5 = dLimit;
	curD = curD - floor(curD);
	
	if (curD < range1)
	{
		curD *= 5.;
		return vec3(1,1,1) * (1. - curD) + vec3(1, 0, .4) * curD;
	}
	else if (curD < range2)
	{
		curD -= range1;
		curD *= 5. ;
		return vec3(1,0,.4) * (1. - curD) + vec3(0, .95, 1) * curD;
	}
	else if (curD < range3)
	{
		curD -= range2;
		curD *= 5.;
		return vec3(0,.95,1) * (1.  - curD) + vec3(0, 1, 0) * curD;
	}
	else if (curD < range4)
	{
		curD -= range3;
		curD *= 5.;
		return vec3(0,1,0) * (1. - curD) + vec3(1, 1, 0) * curD;
	}
	else if (curD < range5)
	{
		curD -= range4;
		curD *=5.;
		return vec3(1,1,0) * (1. - curD) + vec3(1, 1, 1) * curD;
	}

	/*if(curD < 0.2)
	{
		curD * = 5.;
		return vec3(1,1,1) * (1. - curD) + vec3(1, 0, .4) * curD;
	}
	else if(curD < 0.4)
	{
		curD -= 0.2;
		curD *= 5. ;
		return vec3(1,0,.4) * (1. - curD) + vec3(0, .95, 1) * curD;
	}
	else if(curD < 0.6)
	{
		curD -= 0.4;
		curD *= 5.;
		return vec3(0,.95,1) * (1.  - curD) + vec3(0, 1, 0) * curD;
	}
	else if(curD < 0.8)
	{
		curD -= 0.6;
		curD *= 5.;
		return vec3(0,1,0) * (1. - curD) + vec3(1, 1, 0) * curD;
	}
	else if(curD < 1.0)
	{
		curD -= 0.8;
		curD *=5.;
		return vec3(1,1,0) * (1. - curD) + vec3(1, 1, 1) * curD;
	}*/
}


void main()
{
	vec2 temptexcoords;
	if (mod(time * 1000, 1) == 0)
	{
		temptexcoords = vec2(vertex_tex.x*2,vertex_tex.y*2 +0.5);
	}
	else 
	{
		temptexcoords = vec2(vertex_tex.x, vertex_tex.y);
	}
	vec4 tvcol =  texture(tv_tex, temptexcoords);
	color = vec4(tvcol.rgb,1);
	float t = tvcol.a;

	vec4 back_color = vec4(0 + (0.05 * music_influence), 0 , 0, 1);
	float depthcol = texture(depth_tex, temptexcoords).r;
	float f = pow(music_influence * music_influence, 0.5);
	depthcol += time * (0.1 * f);
	vec2 npos = vertex_tex + vec2(time);
	//add functionality to change phi and npos
	float nnn = gold_noise(npos - 1, phi + .43) + (0.15 * music_influence * music_influence);

	vec3 depthcolor = wrongcolor(depthcol*5., kinect_depth);

	vec3 resultcolor = depthcolor * (music_influence*0.4) + vec3(1-depthcol,1-depthcol,1-depthcol)*(1. - music_influence*0.4) *0.4;
	resultcolor.r = pow(resultcolor.r,2);
	resultcolor.g = pow(resultcolor.g,2);
	resultcolor.b = pow(resultcolor.b,2);
	if (depthcol < kinect_depth)
	{
		color = vec4(resultcolor,1);
	}
	else
	{
		color = vec4(resultcolor.r * 0.1, resultcolor.g * 0.1, resultcolor.b * 0.1, 1);
	}
	//color = vec4(resultcolor,1);
	color.rgb *= nnn;
	color = color * (1 - t) + tvcol * t;

	
	return;
}