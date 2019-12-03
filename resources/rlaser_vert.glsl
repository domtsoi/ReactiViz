#version  450 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertTex;
layout(location = 2) in float vertFreq;
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
out vec2 fragTex;
out vec3 ViewPos;
void main()
{
ViewPos = vec3(V * M * vec4(vertPos,1));
vec4 modelpos = M * vec4(vertPos,1);
vec2 direction = modelpos.xy;
modelpos.xy +=direction*vertFreq*0.8;
gl_Position = P * V * modelpos;
fragTex = vertTex;

}