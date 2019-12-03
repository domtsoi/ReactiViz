#version  450 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;
layout (location = 3) in vec4 InstancePos;

layout(location = 0) uniform sampler2D frequ_amplitudes;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform float freq_ampl[10];

out vec3 fragNor;
out vec2 fragTex;
out vec3 fragPos;
out vec4 fragViewPos;
out float textureselect;
out float stretchfact;

uniform vec3 campos;

out float frequampl;





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



void main()
{

vec3 instancepos = vec3(InstancePos.x,0,InstancePos.z);

vec3 pos = instancepos;
pos.y=0;
pos.z +=M[3][2];


float scale = 30;
float height = noise(pos.xzy*scale, 11, 0.03, 0.6);
float baseheight = noise(pos.xzy*scale, 4, 0.004, 0.3);
baseheight = pow(baseheight, 5) * 3;
height = baseheight * height;
height *= 30;
vec4 modelpos = M * vec4(instancepos,1);
height *=min(1,pow(abs(modelpos.x)/10,3));
//audio from texture
vec2 texcoords=pos.xz/50.;
float audioheight = texture(frequ_amplitudes, vec2(texcoords.y,texcoords.x)).r;
height = height*0.2 + height*audioheight*4.0;

//height=0;
//if(pos.x>-5 && pos.x<5)
//if(pos.z<-30 && pos.z>-40)
//height=10;



float scalefact = InstancePos.w;

if(InstancePos.w<0.085)
	{
	height = 1+InstancePos.y;	
	}
else 
	{
	float selectband = InstancePos.w*10.;
	selectband = clamp(selectband,0,9);
	int bandindex = int(selectband);
	scalefact *= 0.8 + freq_ampl[bandindex]*0.05;
	}

vec3 ivertpos = vec3(vertPos.x,vertPos.y,vertPos.z)*scalefact + vec3(instancepos.x,height,instancepos.z);


//ivertpos.y +=0.5*InstancePos.w;


fragPos= (M * vec4(ivertpos, 1.0)).xyz;
fragViewPos=V * vec4(fragPos, 1.0);

gl_Position = P * V * vec4(fragPos, 1.0);
fragNor = (M * vec4(vertNor, 0.0)).xyz;
fragTex = vertTex;
}
