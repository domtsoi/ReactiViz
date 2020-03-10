#version  450 core
#extension GL_ARB_shader_storage_buffer_object : require

layout(local_size_x = 5, local_size_y = 4) in;
layout(binding = 0, offset = 0) uniform atomic_uint ac;

layout (std430, binding = 0) volatile buffer shader_data
{
	vec4 positions_grid[512][424];
};

void main()
{
	positions_grid[0][0] = vec4(1.0f);
}
