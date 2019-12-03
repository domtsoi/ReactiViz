#version 450 core 


layout(location = 0) out vec4 color;
layout(location = 1) out vec4 viewpos;
layout(location = 2) out vec4 worldpos;
layout(location = 3) out vec4 worldnormal;
layout(location = 4) out vec4 godraytex;
layout(location = 5) out vec4 bloommaptarget;

in vec2 frag_tex;
in vec2 rg_frag;
in float maskoffset;
in float fadeout;
layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform sampler2D mask;
void main()
{
	color= texture(tex, frag_tex);
	color.rg += rg_frag;
	//color.rg=frag_tex;
	//color.b=0;
	//color.a=1;
	float maskcol = texture(mask, vec2(frag_tex.x,frag_tex.y*4.) + vec2(0,maskoffset)).x;
	color.a*=maskcol*fadeout;

	viewpos = worldpos = worldnormal = vec4(0,0,0,0);
	float fade = pow(color.a,0.5);
	godraytex=  vec4(color.rgb*fade,color.a);
	bloommaptarget = vec4(0);
}
