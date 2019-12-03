#version  450 core
layout(location = 0) in vec3 vertPos;


uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 offset;
uniform vec3 screen_offset;

layout(location = 0) uniform sampler2D frequ_amplitudes;

out vec3 ViewPos;

float hash(float n) { return fract(sin(n) * 753.5453123); }
float snoise(vec3 x)
	{
	vec3 p = floor(x);
	vec3 f = fract(x);
	f = f * f * (3.0 - 2.0 * f);

	float n = p.x + p.y * 157.0 + 113.0 * p.z;
	return mix(mix(mix(hash(n + 0.0), hash(n + 1.0), f.x),
				mix(hash(n + 157.0), hash(n + 158.0), f.x), f.y),
				mix(mix(hash(n + 113.0), hash(n + 114.0), f.x),
				mix(hash(n + 270.0), hash(n + 271.0), f.x), f.y), f.z);
	}
		//Changing octaves, frequencyand presistance results in a total different landscape.
float noise(vec3 position, int octaves, float frequency, float persistence) 
{
float total = 0.0;
float maxAmplitude = 0.0;
float amplitude = 1.0;
for (int i = 0; i < octaves; i++) 
	{
	total += snoise(position * frequency) * amplitude;
	frequency *= 2.0;
	maxAmplitude += amplitude;
	amplitude *= persistence;
	}
return total / maxAmplitude;
}

		/*I.e. (vertex shader) :
		float height = noise(pos.xzy, 11, 0.03, 0.6);
		float baseheight = noise(pos.xzy, 4, 0.004, 0.3);
		baseheight = pow(baseheight, 5) * 3;
		height = baseheight * height;
		height *= 60;*/

void main()
{

vec3 pos = vertPos + offset;
pos.y=0;
float scale = 30;
float height = noise(pos.xzy*scale, 11, 0.03, 0.6);
float baseheight = noise(pos.xzy*scale, 4, 0.004, 0.3);
baseheight = pow(baseheight, 5) * 3;
height = baseheight * height;
height *= 30;
vec4 modelpos = M * vec4(vertPos,1);
height *=min(1,pow(abs(modelpos.x)/10,3));
//audio from texture
vec2 texcoords=pos.xz/50.;
float audioheight = texture(frequ_amplitudes, vec2(texcoords.y,texcoords.x)).r;
height = height*0.2 + height*audioheight*4.0;
//
//height=0;
//if(pos.x>-5 && pos.x<5)
//if(pos.z<-30 && pos.z>-40)
//height=10;
//



float free_path = min(1,pow(vertPos.x/10,1));



modelpos = M * vec4(vertPos.x,height,vertPos.z,1);

ViewPos = vec3(V * modelpos);

vec4 screenpos =  P * vec4(ViewPos,1);
screenpos.z -=screen_offset.z;
gl_Position = screenpos;
}
