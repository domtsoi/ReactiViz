#version 450 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertTex;

out vec2 vertex_tex;
out vec3 vertex_pos;

void main()
{
	vertex_tex = vertTex;
	vec4 pos = vec4(vertPos, 1.0);

	gl_Position = pos; 
	vertex_tex = vertTex;	
	vertex_pos = pos.xyz;
}
