
#include "Billboard.h"
#include "../Body/Body.h"
#include "../../necessary_includes.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../../stb_image.h"
#define PI (3.1415926f)
#define NUM_JOINTS 25

using namespace std;
using namespace glm;

Billboard::Billboard()
{
	string resource_dir = "../resources/";
	char* uniforms[] = {
		"bodies", "num_bodies", "time_stamps", "time"
	};
	char* attributes[] = {
		"vertPos", "vertTex"
	};
	prog = initProg(resource_dir + "body_vertex.glsl", resource_dir + "body_frag.glsl", uniforms, attributes, 4, 2);
	glUseProgram(prog->pid);
	
	char* texUniforms[] = { "depth_tex", "color_tex", "static_tex" };

	for (int i = 0; i < 3; ++i)
	{
		GLuint TexLocation = glGetUniformLocation(prog->pid, texUniforms[i]);
		glUniform1i(TexLocation, i);
	}

	glUseProgram(0);
	glGenVertexArrays(1, &VAID);
	glBindVertexArray(VAID);
	// set up vbo for postions
	glGenBuffers(1, &POS_BUF);
	glBindBuffer(GL_ARRAY_BUFFER, POS_BUF);
#define WIDTH 1.f
#define HEIGHT 1.f
#define DEPTH  0.f
	vec3 vertices[] = {
		vec3(-WIDTH, -HEIGHT, DEPTH), //ld
		vec3(WIDTH, -HEIGHT, DEPTH), // rd
		vec3(WIDTH, HEIGHT, DEPTH), // ru
		vec3(-WIDTH, HEIGHT, DEPTH), //lu
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	// set up vbo for textures
	glGenBuffers(1, &TEX_BUF);
	glBindBuffer(GL_ARRAY_BUFFER, TEX_BUF);
	vec2 tex[] = {
		vec2(0, 1),
		vec2(1, 1),
		vec2(1, 0),
		vec2(0, 0)
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(tex), tex, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	// set up vbo for indices
	glGenBuffers(1, &IDX_BUF);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IDX_BUF);
	GLushort elements[] = {
		0, 1, 2,
		2, 3, 0,
	};
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
	glBindVertexArray(0);

}
Billboard::Billboard(vector<string> *texts, shared_ptr<Program> program)
{
	prog = program;
	// set up vao
	glGenVertexArrays(1, &VAID);
	glBindVertexArray(VAID);
	// set up vbo for postions
	glGenBuffers(1, &POS_BUF);
	glBindBuffer(GL_ARRAY_BUFFER, POS_BUF);
	vec3 vertices[] = {
		vec3(-1, -1, 0), //ld
		vec3(1, -1, 0), // rd
		vec3(1, 1, 0), // ru
		vec3(-1, 1, 0), //lu
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	// set up vbo for textures
	glGenBuffers(1, &TEX_BUF);
	glBindBuffer(GL_ARRAY_BUFFER, TEX_BUF);
	vec2 tex[] = {
		vec2(0, 1),
		vec2(1, 1),
		vec2(1, 0),
		vec2(0, 0)
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(tex), tex, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	// set up vbo for indices
	glGenBuffers(1, &IDX_BUF);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IDX_BUF);
	GLushort elements[] = {
		0, 1, 2,
		2, 3, 0,
	};
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
	glBindVertexArray(0);

	char filepath[1000];
	TEXTURE_IDS = new GLuint[texts->size()];
	numTexts = texts->size();
	for (int i = 0; i < texts->size(); ++i)
	{
		strcpy(filepath, texts->at(i).c_str());
		handleTexture(filepath, &TEXTURE_IDS[i]);
	}
}

void Billboard::draw(Line line, mat4 *M, mat4 *P, mat4 *V, camera *mycam) {
	glBindVertexArray(VAID);
	prog->bind();
	mat4 Rx = mat4(1); // rotate(mat4(1), PI / 2.f, vec3(1, 0, 0));
	mat4 Ry = rotate(mat4(1), PI / 4.f, vec3(1, 0, 0));
	mat4 Rz;
	for (int i = 0; i < line.pts.size() - 1; ++i) 
	{
		vec3 A = line.pts[i];
		vec3 B = line.pts[i + 1];
		vec3 dir = B - A;
		mat4 T = translate(mat4(1.f), A);
		mat4 S = scale(mat4(1), vec3(1, 1, 1) * (length(dir) / 2.f));
		vec3 ey;
		vec3 ez;
#define EPSILON 0.000001f
		if (dir.z < EPSILON) 
		{
			ey = normalize(dir);
			ez = vec3(0, 0, 1);
			Rx = mat4(1);
			Rz = rotate(mat4(1), PI / 2.f,  vec3(0, 0, 1));
		} 
		else 
		{
			ey = vec3(0, 1, 0);
			ez = normalize(dir);
			Rx = rotate(mat4(1), PI / 2.f, vec3(1, 0, 0));
			Rz = mat4(1);
		}
		vec3 ex = normalize(cross(ey, ex));
		ey = normalize(cross(ez, ex));
		mat4 Rd = mat4(0);
		Rd[0] = vec4(ex, 0);
		Rd[1] = vec4(ey, 0);
		Rd[2] = vec4(ez, 0);
		Rd[3] = vec4(0, 0, 0, 1);
		*M = T * transpose(Rd) * Rx * Rz * S * translate(mat4(1), vec3(0, 1, 0));
		genericDraw(prog, M, P, V, mycam);	
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IDX_BUF);


		for (int i = 0; i < numTexts; ++i) {
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, TEXTURE_IDS[i]);
		}
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void *)0);
		

		
	}
	glBindVertexArray(0);
	prog->unbind();
}

void Billboard::draw(mat4 *M, mat4 *P, mat4 *V, camera *mycam)
{
	prog->bind();
	glBindVertexArray(VAID);
	genericDraw(prog, M, P, V, mycam);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IDX_BUF);


	for (int i = 0; i < numTexts; ++i)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, TEXTURE_IDS[i]);
	}

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void *)0);
	glBindVertexArray(0);

	prog->unbind();
}

double get_elapsed_time()
{
	static double lasttime = glfwGetTime();
	double actualtime = glfwGetTime();
	double difference = actualtime - lasttime;
	lasttime = actualtime;
	return difference;
}

void Billboard::draw(shared_ptr<Frames> frames, shared_ptr<vector<Body>> bodies, shared_ptr<vector<int>> time_stamps)
{
	prog->bind();
	glBindVertexArray(VAID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IDX_BUF);
	time += (get_elapsed_time() / 1000.f);

	for (int i = 0; i < 3; ++i)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, frames->get_text_id(i));
	}

	glUniform4fv(prog->getUniform("bodies"), NUM_JOINTS * bodies->size(), (GLfloat *)bodies->data());
	glUniform1iv(prog->getUniform("time_stamps"), bodies->size(), time_stamps->data());
	glUniform1i(prog->getUniform("num_bodies"), bodies->size());
	glUniform1f(prog->getUniform("time"), time);
	//glUnfirom1i(prog->getUniform("music_influence")) ...
	
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);
	glBindVertexArray(0);

	prog->unbind();
}

void Billboard::draw(shared_ptr<Frames> frames, shared_ptr<vector<int>> time_stamps, int num_bodies)
{
	frames->prog->bind();
	Frames::unbind_render_objects();

	glBindVertexArray(VAID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IDX_BUF);

	for (int i = 0; i < num_bodies; ++i)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, frames->get_render_text_id(i));
	}
	
	glUniform1iv(frames->prog->getUniform("time_stamps"), num_bodies, time_stamps->data());
	glUniform1i(frames->prog->getUniform("num_bodies"), num_bodies);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);
	glBindVertexArray(0);

	frames->prog->unbind();
}

void Billboard::handleTexture(char * filepath, GLuint * Texture, int min_mode) 
{
	int width, height, channels;
	unsigned char * data = stbi_load(filepath, &width, &height, &channels, 4);
	glGenTextures(1, Texture);
	handleTexture(data, Texture, width, height);
}

void Billboard::handleTexture(unsigned char* data, GLuint* Texture, int width, int height, int min_mode)
{
	GLuint TEXTURE_ID = GL_TEXTURE0;
	glActiveTexture(TEXTURE_ID);
	glBindTexture(GL_TEXTURE_2D, *Texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_mode); // _MIPMAP_LINEAR); // GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
}

void Billboard::handleBufferText(GLuint* bufText, GLuint TEXTURE_ID, int width, int height)
{
	glGenTextures(1, bufText);
	glActiveTexture(TEXTURE_ID);
	glBindTexture(GL_TEXTURE_2D, *bufText);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}














