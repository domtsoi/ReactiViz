#version 410 core
out vec4 color;
in vec3 vertex_pos;
in vec2 vertex_tex;
in vec3 vertex_normal;

uniform vec3 col;
uniform sampler2D tex;


void main()
{
	float audio = texture(tex, vertex_tex).r;
	color.g = col.g + audio / 2;
	color.b = col.b + audio / 2;

	if (audio > 0.3)
		color.r = col.r + audio * 4;
	else
		color.r = col.r + audio;

	color.a=1;

}
