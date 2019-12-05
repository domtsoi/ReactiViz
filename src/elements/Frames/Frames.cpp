#include "Frames.h"
#include "../Body/Body.h"

using namespace glm;
using namespace std;

#define NUM_BODIES 6

Frames::Frames(int num_frames)
{
	num_texts = num_frames + 1;
	text_ids = new GLuint[num_texts];

	/*render_objects = new RenderObject[NUM_BODIES];
	for (int i = 0; i < NUM_BODIES; ++i) render_objects[i] = RenderObject();
		string resource_dir = "../resources/";
	char* uniforms[] = {
		"num_bodies", "time_stamps"
	};
	char* attributes[] = {
		"vertPos", "vertTex"
	};
	prog = initProg(resource_dir + "bodies_vertex.glsl", resource_dir + "bodies_frag.glsl", uniforms, attributes, 2, 2);
	glUseProgram(prog->pid);

	char* texUniforms[] = { "body1_tex", "body2_tex", "body3_tex", "body4_tex", "body5_tex", "body6_tex" };

	for (int i = 0; i < 6; ++i)
	{
		GLuint TexLocation = glGetUniformLocation(prog->pid, texUniforms[i]);
		glUniform1i(TexLocation, i);
	}
	*/
	for (int i = 0; i < num_frames; ++i) glGenTextures(1, &text_ids[i]);

	Billboard::handleTexture("../resources/static.png", &text_ids[num_frames]);
	init_compute_prog();

	glUseProgram(0);
}

void Frames::set_text_info(int frame_num, BYTE* data, int width, int height)
{
	Billboard::handleTexture((unsigned char *)data, &text_ids[frame_num], width, height);
}

void Frames::compute_body_depth(shared_ptr<vector<Body>> bodies)
{
	if (bodies->empty()) return;
	glUseProgram(compute_prog);
	// bind active textures
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, text_ids[0]);
	// make an SSBO
	int siz = sizeof(Body) * bodies->size();
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_bodies);
	glBufferData(GL_SHADER_STORAGE_BUFFER, siz, bodies->data(), GL_STATIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_bodies);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
	// Compute Data
	GLuint block_index = 0;
	block_index = glGetProgramResourceIndex(compute_prog, GL_SHADER_STORAGE_BLOCK, "shader_data");
	GLuint ssbo_binding_point_index = 0;
	glShaderStorageBlockBinding(compute_prog, block_index, ssbo_binding_point_index);
	glDispatchCompute(bodies->size() * NUM_JOINTS, (GLuint)1, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	//step 1 getting data back from GPU to the stars object
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_bodies);
	GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	memcpy(bodies->data(), p, siz);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glUseProgram(0);
}

void Frames::init_compute_prog()
{
	//load the compute shader
	std::string ShaderString = readFileAsString("../resources/compute.glsl");
	const char* shader = ShaderString.c_str();
	GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(computeShader, 1, &shader, nullptr);

	GLint rc;
	CHECKED_GL_CALL(glCompileShader(computeShader));
	CHECKED_GL_CALL(glGetShaderiv(computeShader, GL_COMPILE_STATUS, &rc));
	if (!rc)	//error compiling the shader file
	{
		GLSL::printShaderInfoLog(computeShader);
		std::cout << "Error compiling compute shader " << std::endl;
		exit(1);
	}

	compute_prog = glCreateProgram();
	glAttachShader(compute_prog, computeShader);
	glLinkProgram(compute_prog);
	glUseProgram(compute_prog);

	GLuint block_index = 0;
	block_index = glGetProgramResourceIndex(compute_prog, GL_SHADER_STORAGE_BLOCK, "shader_data");
	GLuint ssbo_binding_point_index = 2;
	glShaderStorageBlockBinding(compute_prog, block_index, ssbo_binding_point_index);
	glGenBuffers(1, &ssbo_bodies);

	// bind location of depth_tex in compute prog
	GLuint TexLocation = glGetUniformLocation(compute_prog, "depth_tex");
	glUniform1i(TexLocation, 0);
	glUseProgram(0);
}

Frames::RenderObject::RenderObject()
{
	glGenRenderbuffers(1, &rbo);
	glGenFramebuffers(1, &fbo);
	Billboard::handleBufferText(&tid, GL_TEXTURE0, width, height);
}


void Frames::RenderObject::bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, width, height);
	glBindTexture(GL_TEXTURE_2D, tid);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tid, 0);
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);
}

void Frames::unbind_render_objects()
{
	Frames::RenderObject::unbind();
}

void Frames::RenderObject::unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// TODO: maybe remove next line
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}