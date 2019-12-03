#version 450 core 
layout(location = 0) out vec4 color;
layout(location = 1) out vec4 viewpos;
layout(location = 2) out vec4 worldpos;
layout(location = 3) out vec4 worldnormal;
layout(location = 4) out vec4 godraytex;
layout(location = 5) out vec4 bloommaptarget;

layout(location = 0) uniform sampler2D tex;

in vec2 fragTex;
in vec3 ViewPos;
uniform vec3 extcolor;
void main()
{

vec4 tcolor = texture(tex, fragTex);

color.rgb = tcolor.rgb*extcolor;
color.a=tcolor.a * length(color.rgb);
viewpos = vec4(ViewPos,1);
viewpos.z*=-1;
worldpos = worldnormal = vec4(0,0,0,0);
godraytex= vec4(color);
worldpos.w=1;
bloommaptarget = color;


}
