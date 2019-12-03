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
void main()
{

vec4 ppos = vec4(0);

const float wmul = 5;
const float hmul = 80;

rg_frag = rg[0];

maskoffset =  (age[0]-1.0)*2. -1;

fadeout = clamp(age[0],0.0,1.0);

	vec4 midpos = V * M * gl_in[0].gl_Position;
	vec4 uppos = V * M * (gl_in[0].gl_Position + vec4(0,1,0,0));
	vec3 updir = vec3(uppos - midpos);
	vec3 side = cross(updir,vec3(0,0,1));

	frag_tex=vec2(0,0);
	ppos = P *  (midpos - vec4(side *wmul,1));//vec4(-0.5*wmul,0*hmul,0,1));
	gl_Position =  ppos;
	EmitVertex();
	
	frag_tex=vec2(0,1);
	ppos = P * (midpos - vec4(side *wmul,0) + vec4(updir *hmul,0));
	gl_Position =  ppos;
	EmitVertex();

	frag_tex=vec2(1,1);
	ppos = P * (midpos + vec4(side *wmul,0) + vec4(updir *hmul,0));
	gl_Position =  ppos;
	EmitVertex();
	
	frag_tex=vec2(0,0);
	ppos = P * (midpos - vec4(side *wmul,0));
	gl_Position =  ppos;
	EmitVertex();

	
	frag_tex=vec2(1,1);
	ppos = P * (midpos + vec4(side *wmul,0) + vec4(updir *hmul,0));
	gl_Position =  ppos;
	EmitVertex();

	frag_tex=vec2(1,0);
	ppos = P * (midpos + vec4(side *wmul,0));
	gl_Position =  ppos;
	EmitVertex();

	

}