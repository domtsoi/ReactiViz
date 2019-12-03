#version  450 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
out vec2 fragTex;
out vec3 ViewPos;
void main()
{
ViewPos = vec3(V * M * vec4(vertPos,1));
gl_Position = P * V * M * vec4(vertPos,1);
fragTex = vertTex;

}