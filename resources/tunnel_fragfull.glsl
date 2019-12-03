#version 450 core 
layout(location = 0) out vec4 color;
layout(location = 1) out vec4 viewpos;
layout(location = 2) out vec4 worldpos;
layout(location = 3) out vec4 worldnormal;
layout(location = 4) out vec4 godraytex;
layout(location = 5) out vec4 bloommaptarget;


in vec3 ViewPosT;
in vec3 frag_norm;
in vec3 frag_pos;
uniform vec3 campos;
uniform vec3 roundlaserpos[10];
uniform vec3 roundlasercol[10];
uniform float roundlasercount;

void main()
{
color.rgb = vec3(1,0,0);

int lightcount = int(roundlasercount);
vec3 normal = normalize(frag_norm);
vec3 result = vec3(0);

for(int ii=0;ii<lightcount;ii++)
{
//diffuse light
vec3 lightposition = -roundlaserpos[ii];
vec3 lp = lightposition;
vec3 ld = normalize(lp - frag_pos);
float light = dot(ld,normal);	
light = clamp(light,0,1);

//specular light
vec3 camvec = normalize(frag_pos - campos);
vec3 h = normalize(-camvec+ld);
float spec = pow(dot(h,normal),60);
spec = clamp(spec,0,1)*0.3;

float distancefact = (100. -length(lightposition + frag_pos))/100.;
distancefact = clamp(distancefact,0.0,1.0);


result += ((vec3(0.9,0.9,1.0)*spec+ vec3(0.8,0.8,1.0)*light) *  roundlasercol[ii] + roundlasercol[ii]*light)*distancefact;
}

color.rgb = result;


color.a=1;
viewpos = vec4(ViewPosT,1);
viewpos.z*=-1;
worldpos = worldnormal = vec4(0,0,0,0);
godraytex= bloommaptarget = vec4(0,0,0,1);
worldpos.w=1;
bloommaptarget = vec4(0);
bloommaptarget.a=1;

worldnormal.xyz = normal;
worldnormal.w=0.7;

//if(abs(viewpos.z)>300)
//		{
//		color.a = (400 - abs(viewpos.z))/100.;
//		}
}
