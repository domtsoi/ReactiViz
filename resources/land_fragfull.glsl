#version 450 core 
layout(location = 0) out vec4 color;
layout(location = 1) out vec4 viewpos;
layout(location = 2) out vec4 worldpos;
layout(location = 3) out vec4 worldnormal;
layout(location = 4) out vec4 godraytex;
layout(location = 5) out vec4 bloommaptarget;


in vec3 ViewPos;
void main()
{
color.rgb = vec3(0,0,0);
color.a=1;
viewpos = vec4(ViewPos,1);
viewpos.z*=-1;
worldpos = worldnormal = vec4(0,0,0,0);
godraytex= bloommaptarget = vec4(0,0,0,1);
worldpos.w=1;
bloommaptarget = vec4(0);
bloommaptarget.a=1;
if(abs(viewpos.z)>300)
		{
		color.a = (400 - abs(viewpos.z))/100.;
		}
}
