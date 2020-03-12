#version 330 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

out vec3 vertex_pos;
out vec3 vertex_normal;
out vec2 vertex_tex;

uniform sampler2D tex;
uniform vec3 camoff;

void main()
{
	vec2 texcoords=vertTex;
	float t=1./1000.;

	texcoords -= vec2(camoff.x,camoff.z)*t;

	float audioheight = texture(tex, vec2(texcoords.y,texcoords.x)).r;
	float adjusted = (1.-audioheight*2);

	vec4 tpos = vec4(vertPos, 1.0);
	tpos =  M * tpos;

	tpos.z -= adjusted;

	if (tpos.x > 0)
		tpos.x += adjusted;
	else
		tpos.x -= adjusted;

	if (tpos.y > 0)
		tpos.y += adjusted;
	else
		tpos.y -= adjusted;


	vertex_pos = tpos.xyz;
	gl_Position = P * V * tpos;

	vertex_tex = vertTex;
	vertex_normal = vec4(M * vec4(vertNor,0.0)).xyz;
}
