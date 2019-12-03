#version  450 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;
layout(location = 3) in mat4 InstancePos;

layout(location = 0) uniform sampler2D frequ_amplitudes;

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
out vec3 fragCol1;
out vec3 fragCol2;



void main()
{

float selectband = InstancePos[0].w*10.;
selectband = clamp(selectband,0,9);
int bandindex = int(selectband);
float scalefact = 0.3 + freq_ampl[bandindex]*0.1;

scalefact*=InstancePos[1].w;

vec3 ivertpos = vec3(vertPos.x,vertPos.y,vertPos.z)*scalefact + InstancePos[0].xyz;
fragCol1=InstancePos[1].xyz;
fragCol2=InstancePos[2].xyz;

fragPos= (M * vec4(ivertpos, 1.0)).xyz;
fragViewPos=V * vec4(fragPos, 1.0);

gl_Position = P * V * vec4(fragPos, 1.0);
fragNor = (M * vec4(vertNor, 0.0)).xyz;
fragTex = vertTex;
}
