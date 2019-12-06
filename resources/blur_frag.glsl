#version 450 core 


layout(location = 0) out vec4 color;
in vec2 fragTex;

uniform mat4 Vcam;
uniform mat4 Pcam;
uniform float speed;
uniform float stepno;
uniform float bluractive;

layout(location = 0) uniform sampler2D colortex;
layout(location = 1) uniform sampler2D maskmap;

uniform vec2 windows_size;

const float weights[6] = {
	0.01330373 / 0.47365426,
	0.11098164 / 0.47365426,
	0.22508352 / 0.47365426,
	0.011098164 / 0.47365426,
	0.00330373 / 0.47365426,
	0.001330373 / 0.47365426};


//------------------------------------------------------------------------------
void main()
{
//color.b=0;
//color.rg = fragTex;
//color.a=1;
//return;


float partx = 1./windows_size.x;
float party = 1./windows_size.y;

//some extend for a 10 by 10 blurring
vec3 texturecolor = texture(colortex, fragTex).rgb;
vec4 blurmask = texture(maskmap, fragTex,0);
color.rgb = texturecolor;
color.a=1;

float motion_blur_fact = pow(blurmask.g,0.1);

//distanceblur:
if(bluractive > 0.5)
	{
	vec3 blurcolor=vec3(0);
	
	float motion_blur_fact2 = pow(blurmask.b,1);

	float motion_blur_loop = 1;//motion_blur_fact*20;//*clamp(abs(speed)*6.5,0,1);
	int imotion_blur_loop = int(motion_blur_loop);

	vec2 direction_blur = vec2(0,0);
	if(stepno==0)	direction_blur = vec2(partx,0);
	if(stepno==1)	direction_blur = vec2(0,party);

	
	for(int i = 0; i < 6; i++)
		{
		vec2 toff=direction_blur*i;
		blurcolor += texture(colortex,fragTex + toff).rgb * weights[i];
		}
	for(int i = 0; i < 6; i++)
		{
		vec2 toff=-direction_blur*i;
		blurcolor += texture(colortex,fragTex + toff).rgb * weights[i];
		}

	color.rgb = mix(color.rgb,blurcolor*0.6,motion_blur_fact);	
	}


	//color.rgb=vec3(motion_blur_fact);
}
