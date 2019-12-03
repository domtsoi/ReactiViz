#version 410 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertCol;
layout(location = 2) in vec3 vertDir;
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
out float age;
out vec2 rg;
out vec3 direction;
void main()
{
direction = vertDir;
age = vertCol.x;
rg= vertCol.yz;
vec4 pos = vec4(vertPos.xyz,1.0);
gl_Position = pos;
}
