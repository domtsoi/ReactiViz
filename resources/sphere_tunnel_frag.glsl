#version 450 core 

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 viewpos;
layout(location = 2) out vec4 worldpos;
layout(location = 3) out vec4 worldnormal;
layout(location = 4) out vec4 godraytex;
layout(location = 5) out vec4 bloommaptarget;


in vec3 fragCol1;
in vec3 fragCol2;

in vec3 fragPos;
in vec2 fragTex;
in vec3 fragNor;
in vec4 fragViewPos;
in float textureselect;
in float stretchfact;
uniform vec3 campos;
uniform float reflection_on;
uniform float render_lines;
uniform mat4 V;
uniform float colormode;

in float frequampl;

float angle_vec(vec2 v)
	{
	float w = acos(v.y);
	if (w != 0.0 && v.x<0.)	
		w = (3.1415269*2.0) - w;
	return w;
	}

void main()
{



vec3 normal = normalize(fragNor);

	vec3 reflect_color=vec3(0);
//	if(reflection_on>0.5)textureF=vec3(0);
	worldnormal.xyz = normal;
	worldnormal.w=0;
	color.a=1;
	
	float fact = 1.0 - fragTex.y;
	color.rgb = fragCol1*(1.0 - fact) + fragCol2* fact;
		


	color.rgb *=0.2;
	
	viewpos = fragViewPos;
	viewpos.z*=-1;

	if(abs(viewpos.z)>90)
		{
		color.a = (100 - abs(viewpos.z))/10.;
		}
	worldpos.xyz = fragPos;
	worldpos.w=0;
	godraytex= vec4(0);//color;
	//color = vec4(fragTex.x, -fragTex.y, 0, 1);
	bloommaptarget =  color;
	
}
