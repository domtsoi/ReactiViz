#version 330 core
out vec4 color;
in vec3 vertex_normal;
in vec3 vertex_pos;
in vec2 vertex_tex;

uniform sampler2D tex;
uniform sampler2D tex2;

void main()
{

//// ******************** ORIGINAL ********************
color.rgb = texture(tex2, vertex_tex).rgb;
float audio = 1.0 - texture(tex, vertex_tex).r;
//color.a=1;
color.a=pow(audio, 1);
//// ********************



//color.rgb = texture(tex, vertex_tex).rgb;
////float audio = 1.0 - texture(tex, vertex_tex).r;
//color.a=1;


//color.rgb = vec3(1, 0, 0);
}
