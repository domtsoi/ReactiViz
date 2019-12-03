#version 450 core 


layout(location = 0) out vec4 color;
layout(location = 1) out vec4 viewpos;
layout(location = 2) out vec4 worldpos;
layout(location = 3) out vec4 worldnormal;
layout(location = 4) out vec4 godraytex;
layout(location = 5) out vec4 bloommaptarget;
in vec2 fragTex;

void main()
{
	//vec3 texturecolor = texture(tex, fragTex).rgb;
	color.rgb = vec3(0,0,0);
	color.a=1;
	viewpos = worldpos = worldnormal = vec4(0,0,0,0);
	godraytex= vec4(0,0,0,1);
	bloommaptarget = vec4(0);
}
