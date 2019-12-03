#version 450 core 


layout(location = 0) out vec4 color;
layout(location = 1) out vec4 viewpos;
layout(location = 2) out vec4 worldpos;
layout(location = 3) out vec4 worldnormal;
layout(location = 4) out vec4 godraytex;
layout(location = 5) out vec4 bloommaptarget;
in vec2 fragTex;
in vec3 ViewPos;
layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform sampler2D texgod;
void main()
{
vec2 texcoord = vec2(fragTex.x,fragTex.y*2);
	vec3 texturecolor = texture(tex, texcoord).rgb;
	vec3 texturecolorgod = texture(texgod, texcoord).rgb;
	color.rgb = texturecolor;
	color.a=1;
	viewpos = vec4(ViewPos,1);
	viewpos.z=100.;
	worldpos = worldnormal = vec4(0,0,0,0);
	godraytex= vec4(texturecolorgod,1);
	worldpos.w=0;
	bloommaptarget = vec4(0);
}
