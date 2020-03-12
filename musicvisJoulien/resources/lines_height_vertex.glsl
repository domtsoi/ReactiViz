#version 330 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertTex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
out vec3 vertex_pos;
out vec2 vertex_tex;
uniform sampler2D tex;
uniform sampler2D tex2;
uniform vec3 camoff;

void main()
{
	vec2 texcoords=vertTex;
	float t=1./100.;
	texcoords -= vec2(camoff.x,camoff.z)*t;


	float audioheight = texture(tex, vec2(texcoords.y,texcoords.x)).r;
	
	vec4 tpos =  vec4(vertPos, 1.0);

	tpos.z -=camoff.z;
	tpos =  M * tpos;
	tpos.y += 20* audioheight;

	vertex_pos = tpos.xyz;

	gl_Position = tpos;
	vertex_tex = vertTex;
}
