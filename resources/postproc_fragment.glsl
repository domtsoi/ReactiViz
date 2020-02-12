#version 410 core
out vec4 color;
in vec2 vertex_tex;
uniform sampler2D tex;


void main()
{
vec4 tcol = texture(tex, vertex_tex);

float hd_x = 1./1920.;
float hd_y = 1./1061.;

color = tcol;
color.a=1;

}
