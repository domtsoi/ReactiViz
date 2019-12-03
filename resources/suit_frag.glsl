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

layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform sampler2D tex1;
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
bloommaptarget = vec4(0);
if(render_lines>0.5)
	{
	color = vec4(0,0,1,1);
	return;
	}
vec2 texcoords = fragTex;
texcoords.y *=stretchfact;
	vec3 normal = normalize(fragNor);
	vec3 textureBG = texture(tex, texcoords).rgb;
	float reflection_point = texture(tex1, texcoords).g;
	//vec3 textureF = vec3(0);
	
	//textureF = texture(tex1, texcoords).rgb;

	
	
	vec3 reflect_color=vec3(0);
//	if(reflection_on>0.5)textureF=vec3(0);
	worldnormal.xyz = normal;
	worldnormal.w=0;
	worldnormal.w=1;
	if(reflection_point>0.9)// && textureF.r<0.2)
		{
		
//		vec2 reflec_texcoords = vec2(0);
//		vec3 tofrag = fragPos + campos;
//		vec3 reflect_vec = reflect(tofrag,normal);
//		reflect_vec = normalize(reflect_vec);
//		reflec_texcoords.y = (reflect_vec.y + 1. ) / 2.;	
//		reflec_texcoords.x = -angle_vec(vec2(reflect_vec.x,reflect_vec.z)) / (2.0 *3.1415269)-0.15;
//		vec3 reflecttex = texture(skytex, reflec_texcoords).rgb;
//		reflect_color = reflecttex;


//fragViewPos
		vec3 I = normalize(  fragPos - campos);
		vec3 n = vec3(normal.x,normal.y,normal.z);
		
		vec3 R = reflect(I, n);
//		R.x*=-1;
//		R.z*=-1;
		//R = vec3(inverse(V) * vec4(R, 0.0));
		R = vec3(-R.x,-R.y,-R.z);
		reflect_color = texture(skycube, R).rgb;
		color.rgb =reflect_color*0.7;

		
		color.a=1;
		worldnormal.w=0;
		return;
		}
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
	textureBG*=1.2;
	color.rgb = textureBG*(0.5 + 0.5*light) + vec3(1,1,1)*spec;

	//color.rgb = fragPos;
	//color.rgb =reflect_color*0.7;
	color.a=1;
	viewpos = fragViewPos;
	viewpos.z*=-1;

	if(abs(viewpos.z)>30)
	{
	color.a = (40 - abs(viewpos.z))/10.;
	}
	worldpos.xyz = fragPos;
	worldpos.w=0;

	vec3 lightcolor = vec3(1.0,0.96,0.93);
	lightcolor = vec3(1.0,0.96,0.93);
	color.rgb *=lightcolor;
	//color = vec4(fragTex.x, -fragTex.y, 0, 1);
	godraytex= vec4(0,0,0,1);
}
