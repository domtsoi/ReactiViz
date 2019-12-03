#version  450 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertTex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 offset;


layout(location = 0) uniform sampler2D frequ_amplitudes;
float hash(float n) { return fract(sin(n) * 753.5453123); }


void main()
{
vec3 pos = vertPos + offset;
vec3 direction = normalize(vec3(vertPos.xy,0));
//audio from texture
vec2 texcoords=pos.xz/50.;
texcoords=vec2(hash(vertTex.x),hash(vertTex.y));

float audioheight = texture(frequ_amplitudes, vec2(texcoords.y,texcoords.x)).r;
direction*=-audioheight*10.0;
//direction*=0;
vec4 modelpos = M * vec4(vertPos + direction,1);
gl_Position = modelpos;
}
