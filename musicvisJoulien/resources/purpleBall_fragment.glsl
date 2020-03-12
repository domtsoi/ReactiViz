#version 410 core
out vec4 color;
in vec3 vertex_pos;
in vec2 vertex_tex;
in vec3 vertex_normal;

uniform vec3 campos;
uniform vec3 col;
uniform vec3 lightPos;
uniform vec2 texOff;

uniform sampler2D tex;
uniform sampler2D tex2;
uniform sampler2D tex3;

void main()
{
	vec3 n = normalize(vertex_normal);

	// lightPosition 1
	vec3 lp = lightPos;
	vec3 ld = normalize(vertex_pos - lp);
	float diffuse = dot(n, ld);

	vec3 cd = normalize(vertex_pos - campos);
	vec3 h = normalize(cd+ld);
	float spec = dot(n, h);
	spec = clamp(spec, 0, 1);
	spec = pow(spec, 30);

	// lightPosiiton 2
	vec3 lp2 = vec3(lightPos.x, lightPos.y, lightPos.z);
	vec3 ld2 = normalize(vertex_pos - lp2);
	float diffuse2 = dot(n, ld2);

	vec3 h2 = normalize(cd+ld2);
	float spec2 = dot(n, h2);
	spec2 = clamp(spec2, 0, 1);
	spec2 = pow(spec2, 10);

	// animated cloud/fog texture
	vec2 texCoord = vertex_tex;
	texCoord.x /= 4;
	texCoord.y /= 4;
	texCoord += vec2(0.25 * texOff.x, 0.25 * texOff.y);

	vec3 audio = texture(tex, vertex_tex).rgb;
	vec3 discoTex = texture(tex3, vertex_tex).rgb;
	vec3 cloudTex = texture(tex2,  texCoord).rgb;

	// mute some cloud/fog colors 
	color.rgb = cloudTex / 4;

	float add = audio.r;

	// audio coloring
	if (audio.r > 0.2)
		color.rgb += vec3(col.r +add*4, col.g + add / 2, col.b + add / 2);
	else
		color.rgb += vec3(col.r +add, col.g + add / 2, col.b + add / 2);


	// lighting from two different sources
	diffuse = (0.8 + diffuse * 0.3);
	diffuse2 = (0.7 + diffuse2 * 0.5);
	color.rgb = (color.rgb * diffuse) / 2 + (color.rgb * diffuse2) /2;

	if (color.r < cloudTex.r && color.g < cloudTex.g && color.b < cloudTex.b)
		color.rgb += vec3(1,1,1)*spec2*2.5;


	// add discoball mirror effect
	color.rgb += (discoTex.r + discoTex.g + discoTex.b) / 6 *spec2*5.5;

	// mute some colors
	color.gb /= 2;
	color.a=1;

}
