#version 450 core 


layout(location = 0) out vec4 color;
in vec2 fragTex;

uniform mat4 Vcam;
uniform mat4 Pcam;
uniform float speed;
uniform float stepno;
uniform float bluractive;

layout(location = 0) uniform sampler2D colortex;
layout(location = 1) uniform sampler2D maskmap;

uniform vec2 windows_size;
//
//const float weights[6] = {
//	0.01330373 / 0.97365426,
//	0.11098164 / 0.47365426,
//	0.22508352 / 0.47365426,
//	0.011098164 / 0.47365426,
//	0.00330373 / 0.47365426,
//	0.001330373 / 0.47365426};
//
const float weight[9] = {0.227027,0.2108108, 0.1945946,0.1581081, 0.1216216,0.0878378, 0.054054, 0.034054, 0.016216};



void main()
{
	
    vec2 tex_offset = 1.2 / textureSize(maskmap, 0); // gets size of single texel
  
	int lod = 0;
    if(stepno==0)
    {
	  vec3 result = texture(maskmap, fragTex).rgb*weight[0]; // current fragment's contribution
        for(int i = 1; i < 9; ++i)
        {
            result += texture(maskmap, fragTex + vec2(tex_offset.x * i, 0.0),lod).rgb * weight[i];
            result += texture(maskmap, fragTex - vec2(tex_offset.x * i, 0.0),lod).rgb * weight[i];
        }
	color = vec4(result, 1.0);
    }
    else
    {
	color.rgb = texture(colortex, fragTex).rgb; // current fragment's contribution
	color.a=1;
	//return;
	 vec3 result = texture(maskmap, fragTex).rgb*weight[0]; // current fragment's contribution
       for(int i = 1; i < 9; ++i)
        {
            result += texture(maskmap, fragTex + vec2(0.0, tex_offset.y * i),lod).rgb * weight[i];
            result += texture(maskmap, fragTex - vec2(0.0, tex_offset.y * i),lod).rgb * weight[i];
        }
	//if(color.r<0.001 &&color.g<0.001 &&color.b<0.001 )
	color += vec4(result, 1.0);
    }
  
}