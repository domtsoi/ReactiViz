#version  450 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;
layout (location = 3) in vec4 InstancePos;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform float freq_ampl[10];
out vec3 fragNor;
out vec2 fragTex;
out vec3 fragPos;
out vec4 fragViewPos;
out float textureselect;
out float stretchfact;

uniform vec3 campos;

out float frequampl;
void main()
{
textureselect =  InstancePos.w;

int x = int(InstancePos.x);
int y = int(InstancePos.y);
x =x%20;
y =y%17;
int erg = (x + y)%8;
stretchfact =  InstancePos.y;
float musicfact = 0.01;
if(stretchfact>2.5)musicfact=0.07;
frequampl = freq_ampl[erg];
stretchfact = stretchfact*0.6 + stretchfact*frequampl*musicfact;




vec3 ivertpos = vertPos + vec3(InstancePos.x,0,InstancePos.z);

vec4 realinstancepos = M * vec4(InstancePos.x,0.0,InstancePos.z,1);


//float midabst = length(vec2(realinstancepos.x,realinstancepos.z)-campos.xz);
//if(midabst<2)
//	{
//	float cam_distance_fact = midabst / 2.0;
//	stretchfact *= cam_distance_fact;
//	stretchfact = max(stretchfact,0.5);
//	}

ivertpos.y+=0.5;
fragPos= (M * vec4(ivertpos.x,ivertpos.y*stretchfact,ivertpos.z, 1.0)).xyz;
fragViewPos=V * vec4(fragPos, 1.0);

//




gl_Position = P * V * vec4(fragPos, 1.0);
fragNor = (M * vec4(vertNor, 0.0)).xyz;
fragTex = vertTex;
fragTex.y*=-1;
}
