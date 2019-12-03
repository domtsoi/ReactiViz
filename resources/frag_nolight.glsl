#version 450 core 


layout(location = 0) out vec4 color;
layout(location = 1) out vec4 blurmap;
layout(location = 2) out vec4 bloommaptarget;

in vec2 fragTex;

uniform mat4 Vcam;
uniform mat4 Pcam;
uniform vec2 windows_size;
uniform float speed;

layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform sampler2D viewpos;
layout(location = 2) uniform sampler2D worldpos;
layout(location = 3) uniform sampler2D worldnormal;
layout(location = 4) uniform sampler2D noise;
layout(location = 5) uniform sampler2D skygod;
layout(location = 6) uniform sampler2D bloommap;
layout(location = 7) uniform sampler2D lensdirt;
layout(location = 8) uniform samplerCube skycube;


float CosInterpolate(float v1, float v2, float a)
	{
	float angle = a * 3.1415926;
	float prc = (1.0f - cos(angle)) * 0.5f;
	return  v1*(1.0f - prc) + v2*prc;
	}
vec2 calc_depth_fact(vec2 texcoords)
	{

	float depth = texture(viewpos, texcoords).b;
	//some number magic:
	float processedDepthFact = depth/10.0;
	processedDepthFact = CosInterpolate(0,5,processedDepthFact);
	processedDepthFact = pow(processedDepthFact,2);
	return vec2(depth,processedDepthFact);
	}
//----------------------------------------------------------------------------------
const float stepwidth = 0.1;
const float minRayStep = 0.1;
const float maxSteps = 30;
const int numBinarySearchSteps = 5;
const float reflectionSpecularFalloffExponent = 3.0;
#define Scale vec3(.8, .8, .8)
#define K 19.19
const int NUM_SAMPLES = 100;

vec3 BinarySearch(inout vec3 dir, inout vec3 hitCoord, inout float dDepth)
{
    float depth;

    vec4 projectedCoord;
 
    for(int i = 0; i < numBinarySearchSteps; i++)
    {

        projectedCoord = Pcam * vec4(hitCoord, 1.0);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;
 
 	vec2 coords =  projectedCoord.xy;
		coords.y=1.-coords.y;
		coords.x=1.-coords.x;
        depth = textureLod(viewpos, coords.xy, 2).z;

 
        dDepth = hitCoord.z - depth;

        dir *= 0.5;
        if(dDepth > 0.0)
            hitCoord += dir;
        else
            hitCoord -= dir;    
    }

        projectedCoord = Pcam * vec4(hitCoord, 1.0);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;
 
    return vec3(projectedCoord.xy, depth);
}
//----------------------------------------------------------------------------------
vec4 RayMarch(vec3 dir, inout vec3 hitCoord, out float dDepth)
{
  float depth;
    int steps;
    vec4 projectedCoord;
//
//		projectedCoord = Pcam * vec4(hitCoord, 1.0);
//        projectedCoord.xy /= projectedCoord.w;
//        projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;
//		projectedCoord.x = 1-projectedCoord.x;
//	
//		return vec4(projectedCoord.xy, hitCoord.z, 1.0);

   dir *= 10.55;
 
 
  

 
    for(int i = 0; i < maxSteps; i++)
    {
        hitCoord += dir;
 
        projectedCoord = Pcam * vec4(hitCoord, 1.0);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;
		vec2 coords =  projectedCoord.xy;
		coords.y=1.-coords.y;
		coords.x=1.-coords.x;
       depth = textureLod(viewpos, coords.xy, 2).z;
		
		//return vec4( depth);
		//if(i==29) return vec4(0,0,0, 0.0);
	
       if(depth > 1000.0)
            continue;
	
		//break;
        dDepth = hitCoord.z - depth;

        if((dir.z - dDepth) < 1.2)
        {
            if(dDepth <= 0.0)
            {   
                vec4 Result;
			//	projectedCoord.x = 1-projectedCoord.x;
				//return vec4(projectedCoord.xy, dDepth, 1.0);
				float ddd;
                Result = vec4(BinarySearch(dir, hitCoord, dDepth), ddd);

                return Result;
            }
        }
        
        steps++;
    }
 
   // projectedCoord.x = 1-projectedCoord.x;
    return vec4(projectedCoord.xy, depth, 0.0);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}


vec3 hash(vec3 a)
{
    a = fract(a * Scale);
    a += dot(a, a.yxz + K);
    return fract((a.xxy + a.yxx)*a.zyx);
}
//------------------------------------------------------------------------------
void main()
{



float partx = 1./windows_size.x;
float party = 1./windows_size.y;
//some extend for a 10 by 10 blurring
float arr[]={0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216,0.001,0.0001,0.00001,0.000001,0.0,0.0};
vec3 texturecolor = texture(tex, fragTex).rgb;


vec2 depthfact = calc_depth_fact(fragTex);
vec3 viewPos = textureLod(viewpos, fragTex, 2).xyz;
vec4 worldPos = textureLod(worldpos, fragTex, 2);
vec3 blurcolor = vec3(0,0,0);

	vec3 cubetex = vec3(0);
	cubetex.z=1;
	cubetex.xy = fragTex*2.0 - vec2(1,1);
	//texturecolor = texture(skycube, cubetex).rgb;
	
	
	color.rgb =texturecolor;//texture(worldnormal, fragTex).rgb;//texturecolor;//*0.8 + blurcolor;
//	color.a=1;
//	return;
	float reflecton = texture(worldnormal, fragTex).w;
if(reflecton>0.1)
	{
	vec3 viewNormal = vec3(inverse(Vcam)*vec4(texture2D(worldnormal, fragTex).xyz,0));
    
	
	
    vec3 albedo = texture(tex, fragTex).rgb;

    float spec  = 1;//= texture(ColorBuffer, TexCoords).w;

	float Metallic = 1;
    vec3 F0 = vec3(0.04); 
    F0      = mix(F0, albedo, Metallic);
    vec3 Fresnel = fresnelSchlick(max(dot(normalize(viewNormal), normalize(viewPos)), 0.0), F0);

    // Reflection vector
	//viewNormal.x*=-1;
	//viewNormal.y*=-1;
    vec3 reflected = normalize(reflect(normalize(viewPos), normalize(viewNormal)));

	//reflected=viewNormal;

    vec3 hitPos = viewPos;
    float dDepth;
 
   
    vec3 jitt = mix(vec3(0.0), vec3(hash(worldPos.xyz)), spec);
    vec4 coords = RayMarch((reflected * max(minRayStep, -viewPos.z)), hitPos, dDepth);
// 	
//
//	color.rgb = coords.xyz;
//	color.b=0;
//	color.a=1;
//	return;
 //coords.x = 1-coords.x;
coords.y=1.-coords.y;
	coords.x=1.-coords.x;

    vec2 dCoords = smoothstep(0.2, 0.6, abs(vec2(0.5, 0.5) - fragTex.xy));
 
 
    float screenEdgefactor = clamp(1.0 - (dCoords.x + dCoords.y), 0.0, 1.0);

    float ReflectionMultiplier = pow(Metallic, reflectionSpecularFalloffExponent) * 
                screenEdgefactor * 
                -reflected.z;
 
    // Get color

//	color.rg = coords.xy;
//	color.b=0;
//	color.a=1;
//	return;
    vec3 SSR = textureLod(tex, coords.xy, 0).rgb * Fresnel;  

    color += vec4(SSR, Metallic)*0.4*reflecton;

	}
	
	//color.rgb*=vec3(1,0.6,0.6);
	color.a=1;
	
		const float gamma = 1.2;
  float exposure=0.8;

   
  //vec3 texturecolor2 = texture(tex, fragTex,4).rgb;
  //float noise_check = texture(noise, fragTex).r*0.2;
  //color.rgb=color.rgb*0.8;//+texturecolor2*(0.8+noise_check);
    // reinhard tone mapping
//    vec3 mapped = vec3(1.0) - exp(-color.rgb * exposure);
    // Gamma correction 
    //mapped = pow(mapped, vec3(1.0 / gamma));
  
 // mapped *=vec3(1,0.7,0.8);
  //  color = vec4(mapped, 1.0);
 

 blurmap = vec4(0,0,0,1);
 if(worldPos.w>0.7)
	{
	vec2 tocenter=fragTex-vec2(0.5,0.5);
	if(speed<0)
		tocenter=-tocenter;
	float distance_from_center_fact = length(tocenter)*2.0;//[-1,1]
	distance_from_center_fact = pow(distance_from_center_fact,2);
	float distance_to_pixel_fact = viewPos.z/100.;
	blurmap = vec4(1,distance_to_pixel_fact,distance_from_center_fact,1);
	blurmap = vec4(1,distance_to_pixel_fact,viewPos.z/100.,1);

	}

bloommaptarget = texture(bloommap, fragTex);//*min(1,(1.5 - viewPos.z/100.));

  //godrays

  vec4 light_screen_position = Pcam * Vcam * vec4(0,0,-1000,1);
  light_screen_position.xy /=light_screen_position.w;
  vec2 light_screen_positiont = light_screen_position.xy /2.;
	light_screen_positiont += vec2(0.5,0.5);

    vec2 deltaTextCoord = vec2( fragTex - light_screen_positiont );
    vec2 textCoo = fragTex;
		float density = 0.9f;
	float weight = 2.0f;
	exposure = 0.0034f;
	float decay = 1.0f;
    deltaTextCoord *= 1.0 /  float(NUM_SAMPLES) * density;
    float illuminationDecay = 1.0;

	vec4 godray = vec4(0.0f,0.0f,0.0f,0.0f);

	//Main godray sample loop
    for(int i=0; i < NUM_SAMPLES ; i++)
    {
        textCoo -= deltaTextCoord;
        vec4 sampleGR = texture(skygod, textCoo);
			
        sampleGR *= illuminationDecay * weight;

        godray += sampleGR;

        illuminationDecay *= decay;
    }

    godray *= exposure;

	
	color = (color + godray) /1.0f; 
//
	vec3 LensDirt = texture(lensdirt, fragTex, 0).xyz;
//
	color.rgb += LensDirt*0.1+LensDirt*2.5*godray.rgb;
	//color.rgb = texture(skygod, fragTex).rgb;
}
