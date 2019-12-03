#version 450 core 

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 viewpos;
layout(location = 2) out vec4 worldpos;
layout(location = 3) out vec4 worldnormal;
layout(location = 4) out vec4 godraytex;
layout(location = 5) out vec4 bloommaptarget;

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
in float frequampl;

layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform sampler2D tex1;
layout(location = 2) uniform sampler2D tex2;
layout(location = 3) uniform sampler2D tex3;
layout(location = 4) uniform sampler2D tex4;
layout(location = 5) uniform sampler2D tex5;
layout(location = 6) uniform sampler2D tex6;
layout(location = 7) uniform sampler2D skytex;
layout(location = 8) uniform samplerCube skycube;
float angle_vec(vec2 v)
	{
	float w = acos(v.y);
	if (w != 0.0 && v.x<0.)	
		w = (3.1415269*2.0) - w;
	return w;
	}

void main()
{
//if(render_lines>0.5)
//	{
//	if(stretchfact<1.5)
//		discard;
//	color = vec4(.7,.7,.7,1);
//	return;
//	}

vec3 lightcolor = vec3(1.0,0.96,0.93);
vec2 texcoords = fragTex;
texcoords.y *=stretchfact;
	vec3 normal = normalize(fragNor);
	vec3 textureBG = texture(tex, texcoords).rgb;
	vec3 textureF = vec3(0);
	int texturesel = int(textureselect + 0.001);
	if(texturesel == 0)		textureF = texture(tex1, texcoords*2).rgb;
	else if(texturesel == 1) textureF = texture(tex2, texcoords*2).rgb;
	else if(texturesel == 2) textureF = texture(tex3, texcoords*2).rgb;
	else if(texturesel == 3) textureF = texture(tex4, texcoords*2).rgb;
	else if(texturesel == 4) textureF = texture(tex5, texcoords*2).rgb;
	else if(texturesel == 5) textureF = texture(tex6, texcoords*2).rgb;
	
	float fa =frequampl/3.;
	fa = clamp(fa,0.5,1.0);

	textureF*=fa;
	textureF.r*=0.9;
	textureF.g*=0.9;
	vec3 reflect_color=vec3(0);
//	if(reflection_on>0.5)textureF=vec3(0);
	worldnormal.xyz = normal;
	worldnormal.w=0.5;
	
	//diffuse light
	vec3 lp = vec3(0,0,-1000);
	vec3 ld = normalize(lp - fragPos);
	float light = dot(ld,normal);	
	light = clamp(light,0,1);

	//specular light
	vec3 camvec = normalize(fragPos - campos);
	vec3 h = normalize(-camvec+ld);
	float spec = pow(dot(h,normal),5);
	spec = clamp(spec,0,1)*0.3;
	
	color.rgb = textureBG*0.6*light + vec3(1,1,1)*spec + textureF*0.7 +reflect_color*(light*0.5+0.5)*0.7;

	color.rgb *=lightcolor;
	//color.rgb =reflect_color*0.7;
	color.a=1;
	viewpos = fragViewPos;
	viewpos.z*=-1;

	if(abs(viewpos.z)>90)
		{
		color.a = (100 - abs(viewpos.z))/10.;
		}
	worldpos.xyz = fragPos;
	worldpos.w=1;
	godraytex= vec4(0,0,0,color.a);
	//color = vec4(fragTex.x, -fragTex.y, 0, 1);
	bloommaptarget = vec4(0);
}
