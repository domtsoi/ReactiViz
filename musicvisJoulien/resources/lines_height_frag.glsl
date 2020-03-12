#version 330 core
out vec4 color;
in vec3 frag_pos;
in vec2 frag_tex;
in vec3 frag_norm;

uniform sampler2D tex;
uniform vec3 camoff;
uniform vec3 campos;
uniform vec3 bgcolor;
void main()
{

	float len = length(frag_pos.xz + campos.xz);
	len = (len - 100) / 100.0;
	len = clamp(len,0,1);

	float audioVol = texture(tex, frag_tex).r;

	color.rgb = vec3(1.0,0.5,0.5) * (1 - len) + bgcolor * len;
	color.b += audioVol;
	color.g += audioVol/2;
	color.a = 1;
}
