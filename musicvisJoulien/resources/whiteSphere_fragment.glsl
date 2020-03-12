#version 410 core
out vec4 color;
in vec3 vertex_pos;
in vec2 vertex_tex;
in vec3 vertex_normal;

uniform vec3 col;
uniform sampler2D tex;
uniform sampler2D tex2;


void main()
{
	color.rgb = col/1.5;
	color.a=0.7;
}
