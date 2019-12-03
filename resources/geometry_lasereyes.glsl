#version 450 core

layout(points) in;
layout(triangle_strip, max_vertices = 6) out;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
out vec2 frag_tex;
out vec2 rg_frag;
out float maskoffset;
out float fadeout;
in float age[];
in vec2 rg[];
in vec3 direction[];
void main()
{

vec4 ppos = vec4(0);

const float wmul = 25;
const float hmul = 800;

rg_frag = rg[0];

maskoffset =  (age[0]-1.0)*2. -1;

fadeout = clamp(age[0],0.0,1.0);

	vec4 midpos = V * M * gl_in[0].gl_Position;
	
	vec4 frontpos = V * M * (gl_in[0].gl_Position + vec4(direction[0],0));
	vec3 frontdir = normalize(vec3(frontpos - midpos));
	vec3 up = normalize(cross(frontdir,normalize(-midpos.xyz)))*pow(fadeout,5);

	frag_tex=vec2(0,0);
	ppos = P *  (midpos - vec4(up *wmul,1));//vec4(-0.5*wmul,0*hmul,0,1));
	gl_Position =  ppos;
	EmitVertex();
	
	frag_tex=vec2(0,1);
	ppos = P * (midpos - vec4(up *wmul,0) + vec4(frontdir *hmul,0));
	gl_Position =  ppos;
	EmitVertex();

	frag_tex=vec2(1,1);
	ppos = P * (midpos + vec4(up *wmul,0) + vec4(frontdir *hmul,0));
	gl_Position =  ppos;
	EmitVertex();
	
	frag_tex=vec2(0,0);
	ppos = P * (midpos - vec4(up *wmul,0));
	gl_Position =  ppos;
	EmitVertex();

	
	frag_tex=vec2(1,1);
	ppos = P * (midpos + vec4(up *wmul,0) + vec4(frontdir *hmul,0));
	gl_Position =  ppos;
	EmitVertex();

	frag_tex=vec2(1,0);
	ppos = P * (midpos + vec4(up *wmul,0));
	gl_Position =  ppos;
	EmitVertex();

	

}