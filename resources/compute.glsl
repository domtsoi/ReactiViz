#version 450
#extension GL_ARB_shader_storage_buffer_object : require
layout(local_size_x = 1, local_size_y = 1) in;
const int num_joints = 25;
const int num_bodies = 6;
layout(binding=0) volatile buffer shader_data
{
   vec4 joints[];
};

uniform sampler2D depth_tex;

void main()
{
	uint index = gl_GlobalInvocationID.x;
	// TODO: figure out if .xy is good enough for cooridnates
	joints[index].z = texture(depth_tex, joints[index].xy).r;
}