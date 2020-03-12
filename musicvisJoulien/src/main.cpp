/*
musicvisualizer
by Joulien Ivanov 2019
Based on CPE CSC 471 music visualizer base code
On Windows, it whould capture "what you hear" automatically, as long as you have the Stereo Mix turned on!! (Recording devices -> activate)
*/

#include <iostream>
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "recordAudio.h"
#include "WindowManager.h"
#include "Shape.h"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <thread>
using namespace std;
using namespace glm;
shared_ptr<Shape> jShape;
#define MESHSIZE 200
extern captureAudio actualAudioData;
extern int running;

int renderstate = 1;//2..grid

#define TEXSIZE 256
BYTE texels[TEXSIZE*TEXSIZE*4];
//********************
#include "fftw/fftw3.h"
#include <math.h>
#include <algorithm>    

#define FFTW_ESTIMATEE (1U << 6)

fftw_complex * fft(int &length)
{
	static fftw_complex *out = NULL;
	if (out)delete[] out;
	int N = pow(2, 10);
	BYTE data[MAXS];
	int size = 0;
	actualAudioData.readAudio(data, size);
	length = size / 8;
	if (size == 0)
		return NULL;

	double *samples = new double[length];
	for (int ii = 0; ii < length; ii++)
	{
		float *f = (float*)&data[ii * 8];
		samples[ii] = (double)(*f);
	}
	out = new fftw_complex[length];
	fftw_plan p;
	fftw_complex fout[10000];
	p = fftw_plan_dft_r2c_1d(length, samples, out, FFTW_ESTIMATEE);

	fftw_execute(p);
	fftw_destroy_plan(p);

	return out;
}
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

BYTE delayfilter(BYTE old, BYTE actual, float mul)
{
	float fold = (float)old;
	float factual = (float)actual;
	float fres = fold - (fold - factual) / mul;
	if (fres > 255) fres = 255;
	else if (fres < 0)fres = 0;
	return (BYTE)fres;
}

void write_to_tex(GLuint texturebuf,int resx,int resy)
{
	int length;
	static float arg = 0;
	arg += 0.1;
	float erg = sin(arg) + 1;
	erg *= 100;
	int y = 0;
	fftw_complex *outfft = fft(length);
	float fm = 0;
	//low
	for (int x = 0; x < resx; ++x)
		{
		if (x >= length/2)break;
		float fftd = sqrt(outfft[x][0]* outfft[x][0]+ outfft[x][1]* outfft[x][1]);
		fm = max(fm, abs(fftd));
		BYTE oldval = texels[x * 4 + y*resx * 4 + 0];
		texels[x * 4 + y*resx * 4 + 0] = delayfilter(oldval,(BYTE)(fftd*60.0),15);
		texels[x * 4 + y*resx * 4 + 1] = (BYTE)erg;
		texels[x * 4 + y*resx * 4 + 2] = (BYTE)erg;
		texels[x * 4 + y*resx * 4 + 3] = (BYTE)erg;
		}
	//high
	for (int y = 0; y < resx; ++y)
	{
		if ((y+resx) >= length / 2)break;
		float fftd = sqrt(outfft[y + resx][0] * outfft[y + resx][0] + outfft[y + resx][1] * outfft[y + resx][1]);
		fm = max(fm, abs(fftd));
		BYTE oldval = texels[y*resx * 4 + 0];
		texels[ y*resx * 4 + 0] = delayfilter(oldval, (BYTE)(fftd*60.0), 15);
		texels[ y*resx * 4 + 1] = (BYTE)erg;
		texels[ y*resx * 4 + 2] = (BYTE)erg;
		texels[ y*resx * 4 + 3] = (BYTE)erg;
	}
	
	for (int y = 1; y <resy; y++)
		for (int x = 1; x < resx; ++x)
			{
			int erg = (int)texels[(y - 1)*resx * 4] + (int)texels[x * 4];
			erg /= 3;
			texels[x * 4 + y*resx * 4] = erg;
			}
	//cout << length << endl;
	//fftw_complex fout[10000];
	//memcpy(fout, outfft, sizeof(fftw_complex)*length);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texturebuf);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEXSIZE, TEXSIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, texels);
	glGenerateMipmap(GL_TEXTURE_2D);

	//cout << "audio" << endl;
}

double get_last_elapsed_time()
{
	static double lasttime = glfwGetTime();
	double actualtime =glfwGetTime();
	double difference = actualtime- lasttime;
	lasttime = actualtime;
	return difference;
}
class camera
{
public:
	glm::vec3 pos, rot;
	int w, a, s, d, q, e, z, c;
	camera()
	{
		w = a = s = d = q = e = z = c = 0;
		rot = glm::vec3(0, 0, 0);
		pos = vec3(0, 0, -20);
	}
	glm::mat4 process(double ftime)
	{
		float speed = 0;
		if (w == 1)
		{
			speed = 10*ftime;
		}
		else if (s == 1)
		{
			speed = -10*ftime;
		}
		float yangle=0;
		if (a == 1)
			yangle = -5*ftime;
		else if(d==1)
			yangle = 5*ftime;
		rot.y += yangle;
		float zangle = 0;
		if (q == 1)
			zangle = -5 * ftime;
		else if (e == 1)
			zangle = 5 * ftime;
		rot.z += zangle;
		float xangle = 0;
		if (z == 1)
			xangle = -0.7 * ftime;
		else if (c == 1)
			xangle = 0.7 * ftime;
		rot.x += xangle;

		glm::mat4 R = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
		glm::mat4 Rz = glm::rotate(glm::mat4(1), rot.z, glm::vec3(0, 0, 1));
		glm::mat4 Rx = glm::rotate(glm::mat4(1), rot.x, glm::vec3(1, 0, 0));
		glm::vec4 dir = glm::vec4(0, 0, speed,1);
		R = Rx * Rz * R;
		dir = dir*R;
		pos += glm::vec3(dir.x, dir.y, dir.z);
		glm::mat4 T = glm::translate(glm::mat4(1), pos);
		return R*T;
	}
};

camera mycam;

class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> jProg, jLineShader, jBackground, jPurpleSphere, jWhiteSphere;

	// Data necessary to give our box to OpenGL
	GLuint VertexBufferID, VertexNormDBox, VertexTexBox;
	GLuint meshVAO, MeshPosID, IndexBufferIDBox, MeshTexID;

	//texture data
	GLuint Texture, AudioTex, AudioTexBuf;
	GLuint Texture2,Texture3;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if (key == GLFW_KEY_R && action == GLFW_PRESS)
		{
			mycam.pos = vec3(0, 0, -20);
			mycam.rot = glm::vec3(0, 0, 0);
		}
		if (key == GLFW_KEY_W && action == GLFW_PRESS)
		{
			mycam.w = 1;
		}
		if (key == GLFW_KEY_W && action == GLFW_RELEASE)
		{
			mycam.w = 0;
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS)
		{
			mycam.s = 1;
		}
		if (key == GLFW_KEY_S && action == GLFW_RELEASE)
		{
			mycam.s = 0;
		}
		if (key == GLFW_KEY_A && action == GLFW_PRESS)
		{
			mycam.a = 1;
		}
		if (key == GLFW_KEY_A && action == GLFW_RELEASE)
		{
			mycam.a = 0;
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS)
		{
			mycam.d = 1;
		}
		if (key == GLFW_KEY_D && action == GLFW_RELEASE)
		{
			mycam.d = 0;
		}
		if (key == GLFW_KEY_Q && action == GLFW_PRESS)
		{
			mycam.q = 1;
		}
		if (key == GLFW_KEY_Q && action == GLFW_RELEASE)
		{
			mycam.q = 0;
		}
		if (key == GLFW_KEY_E && action == GLFW_PRESS)
		{
			mycam.e = 1;
		}
		if (key == GLFW_KEY_E && action == GLFW_RELEASE)
		{
			mycam.e = 0;
		}
		if (key == GLFW_KEY_Z && action == GLFW_PRESS)
		{
			mycam.z = 1;
		}
		if (key == GLFW_KEY_Z && action == GLFW_RELEASE)
		{
			mycam.z = 0;
		}
		if (key == GLFW_KEY_C && action == GLFW_PRESS)
		{
			mycam.c = 1;
		}
		if (key == GLFW_KEY_C && action == GLFW_RELEASE)
		{
			mycam.c = 0;
		}
		if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
		{
			if (renderstate == 1)
				renderstate = 2;
			else
				renderstate = 1;
		}
		
	}


	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
	}

	////if the window is resized, capture the new size and reset the viewport
	void resizeCallback(GLFWwindow *window, int in_width, int in_height)
	{
		//get the window size - may be different then pixels for retina
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
	}

	void init_mesh()
	{
		//generate the VAO
		glGenVertexArrays(1, &meshVAO);
		glBindVertexArray(meshVAO);

		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &MeshPosID);
		glBindBuffer(GL_ARRAY_BUFFER, MeshPosID);
		glm::vec3* vertices = new glm::vec3[MESHSIZE * MESHSIZE * 6];

		float alpha = 0, rad = 20;
		float alphaAdd = 2 * 3.1415 / MESHSIZE;
		float xCirc, y;

		for (int x = 0; x < MESHSIZE; x++)
		{
			for (int z = 0; z < MESHSIZE; z++)
			{
				y = rad * -sin(alpha);
				xCirc = rad * cos(alpha);
				vertices[x * 6 + z * MESHSIZE * 6 + 0] = vec3(0.0, 0.0, 0.0) + vec3(xCirc, y, z);//LD
				vertices[x * 6 + z * MESHSIZE * 6 + 1] = vec3(1.0, 0.0, 0.0) + vec3(xCirc, y, z);//RD
				vertices[x * 6 + z * MESHSIZE * 6 + 2] = vec3(1.0, 0.0, 1.0) + vec3(xCirc, y, z);//RU
				vertices[x * 6 + z * MESHSIZE * 6 + 3] = vec3(0.0, 0.0, 0.0) + vec3(xCirc, y, z);//LD
				vertices[x * 6 + z * MESHSIZE * 6 + 4] = vec3(1.0, 0.0, 1.0) + vec3(xCirc, y, z);//RU
				vertices[x * 6 + z * MESHSIZE * 6 + 5] = vec3(0.0, 0.0, 1.0) + vec3(xCirc, y, z);//LU

			}
				alpha += alphaAdd;

		}

		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * MESHSIZE * MESHSIZE * 6, vertices, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		delete[] vertices;
		//tex coords
		float t = 1. / MESHSIZE;
		vec2* tex = new vec2[MESHSIZE * MESHSIZE * 6];
		for (int x = 0; x < MESHSIZE; x++)
			for (int y = 0; y < MESHSIZE; y++)
			{
				tex[x * 6 + y * MESHSIZE * 6 + 0] = vec2(0.0, 0.0) + vec2(x, y) * t;	//LD
				tex[x * 6 + y * MESHSIZE * 6 + 1] = vec2(t, 0.0) + vec2(x, y) * t;		//RD
				tex[x * 6 + y * MESHSIZE * 6 + 2] = vec2(t, t) + vec2(x, y) * t;		//RU
				tex[x * 6 + y * MESHSIZE * 6 + 3] = vec2(0.0, 0.0) + vec2(x, y) * t;	//LD
				tex[x * 6 + y * MESHSIZE * 6 + 4] = vec2(t, t) + vec2(x, y) * t;		//RU
				tex[x * 6 + y * MESHSIZE * 6 + 5] = vec2(0.0, t) + vec2(x, y) * t;		//LU
			}
		glGenBuffers(1, &MeshTexID);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, MeshTexID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * MESHSIZE * MESHSIZE * 6, tex, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		delete[] tex;
		glGenBuffers(1, &IndexBufferIDBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferIDBox);
		GLuint* elements = new GLuint[MESHSIZE * MESHSIZE * 8];
		int ind = 0, i = 0;
		for (i = 0; i < (MESHSIZE * MESHSIZE * 8); i += 8, ind += 6)
		{
			elements[i + 0] = ind + 0;
			elements[i + 1] = ind + 1;
			elements[i + 2] = ind + 1;
			elements[i + 3] = ind + 2;
			elements[i + 4] = ind + 2;
			elements[i + 5] = ind + 5;
			elements[i + 6] = ind + 5;
			elements[i + 7] = ind + 0;
		}
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * MESHSIZE * MESHSIZE * 8, elements, GL_STATIC_DRAW);
		delete[] elements;
		glBindVertexArray(0);
	}


	/*Note that any gl calls must always happen after a GL state is initialized */
	void initGeom()
	{
		init_mesh();

		// other objects
		string resourceDirectory = "../resources" ;
		jShape = make_shared<Shape>();
		jShape->loadMesh(resourceDirectory + "/sphere.obj");
		jShape->resize();
		jShape->init();

		int width, height, channels;
		char filepath[1000];

		//texture 1
		string str = resourceDirectory + "/sky.jpg";
		strcpy(filepath, str.c_str());
		unsigned char* data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
			         GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		//texture 2
		str = resourceDirectory + "/clouds.jpg"; // "/fire.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture2);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, Texture2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		//texture 3
		str = resourceDirectory + "/disco.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture3);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, Texture3);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);


		////[TWOTEXTURES]
		//set the 2 textures to the correct samplers in the fragment shader:
		GLuint Tex1Location = glGetUniformLocation(jProg->pid, "tex");//tex, tex2... sampler in the fragment shader
		GLuint Tex2Location = glGetUniformLocation(jProg->pid, "tex2");
		// Then bind the uniform samplers to texture units:
		glUseProgram(jProg->pid);
		glUniform1i(Tex1Location, 0);
		glUniform1i(Tex2Location, 1);


		////[TWOTEXTURES]
		Tex1Location = glGetUniformLocation(jBackground->pid, "tex");
		Tex2Location = glGetUniformLocation(jBackground->pid, "tex2");
		glUseProgram(jBackground->pid);
		glUniform1i(Tex1Location, 0);
		glUniform1i(Tex2Location, 1);

		////[TWOTEXTURES]
		Tex1Location = glGetUniformLocation(jPurpleSphere->pid, "tex");
		Tex2Location = glGetUniformLocation(jPurpleSphere->pid, "tex2");
		GLuint Tex3Location = glGetUniformLocation(jPurpleSphere->pid, "tex3");

		glUseProgram(jPurpleSphere->pid);
		glUniform1i(Tex1Location, 0);
		glUniform1i(Tex2Location, 1);
		glUniform1i(Tex3Location, 2);


		////[TWOTEXTURES]
		Tex1Location = glGetUniformLocation(jWhiteSphere->pid, "tex");
		Tex2Location = glGetUniformLocation(jWhiteSphere->pid, "tex2");
		glUseProgram(jWhiteSphere->pid);
		glUniform1i(Tex1Location, 0);
		glUniform1i(Tex2Location, 1);


		// dynamic audio texture
		for (int ii = 0; ii < TEXSIZE*TEXSIZE * 4; ii++)
			texels[ii] = 0;
		glGenTextures(1, &AudioTex);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, AudioTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEXSIZE, TEXSIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, texels);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	
	//General OGL initialization - set OGL state here
	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);


		// Initialize the GLSL program.
		jProg = std::make_shared<Program>();
		jProg->setVerbose(true);
		jProg->setShaderNames(resourceDirectory + "/shader_vertex.glsl", resourceDirectory + "/shader_fragment.glsl");
		if (!jProg->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		jProg->addUniform("P");
		jProg->addUniform("V");
		jProg->addUniform("M");
		jProg->addUniform("camoff");
		jProg->addAttribute("vertPos");
		jProg->addAttribute("vertNor");
		jProg->addAttribute("vertTex");
	
		// Initialize the GLSL program.
		jBackground = std::make_shared<Program>();
		jBackground->setVerbose(true);
		jBackground->setShaderNames(resourceDirectory + "/shader_vertex2.glsl", resourceDirectory + "/shader_fragment2.glsl");
		if (!jBackground->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		jBackground->addUniform("P");
		jBackground->addUniform("V");
		jBackground->addUniform("M");
		jBackground->addUniform("col");
		jBackground->addAttribute("vertPos");
		jBackground->addAttribute("vertNor");
		jBackground->addAttribute("vertTex");


		// Initialize the GLSL program.
		jPurpleSphere = std::make_shared<Program>();
		jPurpleSphere->setVerbose(true);
		jPurpleSphere->setShaderNames(resourceDirectory + "/shader_vertex2.glsl", resourceDirectory + "/purpleBall_fragment.glsl");
		if (!jPurpleSphere->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		jPurpleSphere->addUniform("P");
		jPurpleSphere->addUniform("V");
		jPurpleSphere->addUniform("M");
		jPurpleSphere->addUniform("col");
		jPurpleSphere->addUniform("lightPos");
		jPurpleSphere->addUniform("texOff");
		jPurpleSphere->addAttribute("vertPos");
		jPurpleSphere->addAttribute("vertNor");
		jPurpleSphere->addAttribute("vertTex");

		// Initialize the GLSL program.
		jWhiteSphere = std::make_shared<Program>();
		jWhiteSphere->setVerbose(true);
		jWhiteSphere->setShaderNames(resourceDirectory + "/shader_vertex2.glsl", resourceDirectory + "/whiteSphere_fragment.glsl");
		if (!jWhiteSphere->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		jWhiteSphere->addUniform("P");
		jWhiteSphere->addUniform("V");
		jWhiteSphere->addUniform("M");
		jWhiteSphere->addUniform("col");
		jWhiteSphere->addAttribute("vertPos");
		jWhiteSphere->addAttribute("vertNor");
		jWhiteSphere->addAttribute("vertTex");

		// mesh program
		jLineShader = std::make_shared<Program>();
		jLineShader->setVerbose(true);
		jLineShader->setShaderNames(resourceDirectory + "/lines_height_vertex.glsl", resourceDirectory + "/lines_height_frag.glsl", resourceDirectory + "/lines_geometry.glsl");
		if (!jLineShader->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		jLineShader->addUniform("P");
		jLineShader->addUniform("V");
		jLineShader->addUniform("M");
		jLineShader->addUniform("camoff");
		jLineShader->addUniform("campos");
		jLineShader->addAttribute("vertPos");
		jLineShader->addAttribute("vertTex");
		jLineShader->addUniform("bgcolor");
	}


	/**** DRAW ****/
	void render()
	{
		static float totalTime = 0;
		double frametime = get_last_elapsed_time();
		write_to_tex(AudioTex, TEXSIZE, TEXSIZE);

		totalTime += frametime;


		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Create the matrix stacks - please leave these alone for now
		glm::mat4 V, M, P; //View, Model and Perspective matrix

		V = mycam.process(frametime);
		M = glm::mat4(1);
		P = glm::perspective((float)(3.14159 / 4.), (float)((float)width / (float)height), 0.01f, 100000.0f); //so much type casting... GLM metods are quite funny ones

		//animation with the model matrix:
		static float w = 0.0;
		//static float lightRot = sin(w) * 2;
		w += 1.0 * frametime;//rotation angle

		glm::mat4 RotateY, RotateX, TransZ, S, lightPos;
		float angle = 3.1415926 / 2.0;
		static float sangle = 1, zangle = 0;
		static vec3 color = vec3(0, 0, 0);

		vec3 offset = mycam.pos;
		offset.y = 0;
		offset.x = (int)offset.x;
		offset.z = (int)offset.z;

		if (renderstate == 1)
		{
			mat4 parentTranslate = translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 21));

			//// **************** RED BACKGROUND VIZUALIZER ****************
			{
				jBackground->bind();
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

				glm::mat4 RotateZSky = glm::rotate(glm::mat4(1.0f), zangle, glm::vec3(0.0f, 0.0f, 1.0f));
				glm::vec3 camp = -mycam.pos;
				glm::mat4 TransSky = glm::translate(glm::mat4(1.0f), camp);
				glm::mat4 SSky = glm::scale(glm::mat4(1.0f), glm::vec3(0.8f, 0.8f, 0.8f));
				zangle += 0.001;
				M = TransSky * RotateZSky * SSky;

				//send the matrices to the shaders
				glUniformMatrix4fv(jBackground->getUniform("P"), 1, GL_FALSE, &P[0][0]);
				glUniformMatrix4fv(jBackground->getUniform("V"), 1, GL_FALSE, &V[0][0]);
				glUniformMatrix4fv(jBackground->getUniform("M"), 1, GL_FALSE, &M[0][0]);
				glUniform3fv(jBackground->getUniform("col"), 1, &vec3(0.0, 0.0, 0.0)[0]);
		
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, AudioTex);

				glDisable(GL_DEPTH_TEST);
				jShape->draw(jBackground, FALSE);
				jBackground->unbind();
			}

			//// ************************ VISUALIZERS *****************************
				//// **************** PARTICLES ****************
			{
					jProg->bind();

					// outer spread out particles
					glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
					RotateY = glm::rotate(glm::mat4(1.0f), w, glm::vec3(0.0f, 1.0f, 0.0f));
					RotateX = glm::rotate(glm::mat4(1.0f), sangle, glm::vec3(1.0f, 0.0f, 0.0f));
					TransZ = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5));
					S = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f / 4, 1.0f));
					M = parentTranslate * TransZ *  S;

					glUniformMatrix4fv(jProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
					glUniformMatrix4fv(jProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
					glUniformMatrix4fv(jProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
					glUniform3fv(jProg->getUniform("camoff"), 1, &offset[0]);

					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, AudioTex);
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, Texture);

					jShape->draw(jProg, FALSE);

					glEnable(GL_CULL_FACE);
					glFrontFace(GL_CCW);

					// smaller, closer and rotating particles
					RotateX = glm::rotate(glm::mat4(1.0f), sangle, glm::vec3(1.0f, 0.0f, 0.0f));
					TransZ = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5));
					S = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f/4, 1.0f/4, 1.0f/4));
					M = parentTranslate * TransZ * RotateX * S;
					glUniformMatrix4fv(jProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
					glUniformMatrix4fv(jProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
					glUniformMatrix4fv(jProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
					glUniform3fv(jProg->getUniform("camoff"), 1, &offset[0]);

					jShape->draw(jProg, FALSE);
					glDisable(GL_CULL_FACE);
			}


				//// **************** CENTER CIRCLE ****************
			{
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					TransZ = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5));
					S = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));
					M = parentTranslate * TransZ * RotateX * S;
					sangle += 0.02;

					glUniformMatrix4fv(jProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
					glUniformMatrix4fv(jProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
					glUniformMatrix4fv(jProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
					glUniform3fv(jProg->getUniform("camoff"), 1, &offset[0]);

					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, Texture);
					jShape->draw(jProg, FALSE);


					glEnable(GL_DEPTH_TEST);
			}
		}

		else
		{
		//// **************** pulsating SKY SPHERE **************** (used to be white hence the prog name)
			{
				jWhiteSphere->bind();
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

				//glm::mat4 RotateZSky = glm::rotate(glm::mat4(1.0f), zangle, glm::vec3(0.0f, 0.0f, 1.0f));
				glm::vec3 camp = -mycam.pos;
				glm::mat4 TransSky = glm::translate(glm::mat4(1.0f), camp);
				glm::mat4 SSky = glm::scale(glm::mat4(1.0f), glm::vec3(0.8f, 0.8f, 0.8f));
				M = TransSky * SSky;

				color += (sin(w)/2.f - 0.7);

				//send the matrices to the shaders
				glUniformMatrix4fv(jWhiteSphere->getUniform("P"), 1, GL_FALSE, &P[0][0]);
				glUniformMatrix4fv(jWhiteSphere->getUniform("V"), 1, GL_FALSE, &V[0][0]);
				glUniformMatrix4fv(jWhiteSphere->getUniform("M"), 1, GL_FALSE, &M[0][0]);
				glUniform3fv(jWhiteSphere->getUniform("col"), 1, &color[0]);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, AudioTex);

				glDisable(GL_DEPTH_TEST);
				jShape->draw(jWhiteSphere, FALSE);
				jWhiteSphere->unbind();
			}

			glEnable(GL_DEPTH_TEST);

			//// **************** MESH TUBE ****************
			{
				// add mesh to scene
				jLineShader->bind();
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glm::mat4 TransY = glm::translate(glm::mat4(1.0f), glm::vec3(0, -5.0f, MESHSIZE * -1.f));	//MESHSIZE/-2.0f
				M = TransY;
				glUniformMatrix4fv(jLineShader->getUniform("M"), 1, GL_FALSE, &M[0][0]);
				glUniformMatrix4fv(jLineShader->getUniform("P"), 1, GL_FALSE, &P[0][0]);
				glUniformMatrix4fv(jLineShader->getUniform("V"), 1, GL_FALSE, &V[0][0]);

				vec3 bg = vec3(color.r / 2, color.g / 2, color.b / 2);
				glUniform3fv(jLineShader->getUniform("camoff"), 1, &offset[0]);
				glUniform3fv(jLineShader->getUniform("campos"), 1, &mycam.pos[0]);
				glUniform3fv(jLineShader->getUniform("bgcolor"), 1, &bg[0]);

				glBindVertexArray(meshVAO);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, AudioTex);

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferIDBox);
				glDrawElements(GL_LINES, MESHSIZE * MESHSIZE * 8, GL_UNSIGNED_INT, (void*)0);
				jLineShader->unbind();
			}


			//// **************** PURPLE SPHERE VIZUALIZER ****************
			{
				jPurpleSphere->bind();
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


				glm::mat4 RotateXSky = glm::rotate(glm::mat4(1.0f), 1.f, glm::vec3(1.0f, 0.0f, 0.0f));
				glm::mat4 RotateYSky = glm::rotate(glm::mat4(1.0f), zangle, glm::vec3(0.0f, 1.0f, 0.0f));
				glm::vec3 camp = -mycam.pos;
				camp.z -= 8;
				camp.y -= 0.2;
				camp.x += 0.03;

				glm::mat4 TransSky = glm::translate(glm::mat4(1.0f), camp);
				glm::mat4 SSky = glm::scale(glm::mat4(1.0f), glm::vec3(0.75f, 0.75f, 0.75f));
				zangle += 0.005;

				mat4 parent = TransSky * RotateYSky * RotateXSky;
				M = parent * SSky;

				// moving light source
				TransSky = translate(mat4(1.0f), vec3(4.0f, 0.0f, -2.0f));
				RotateXSky = rotate(mat4(1.0f), w, vec3(1.0f, 1.0f, 1.0f));
				lightPos = parent * RotateXSky * TransSky;


				vec3 lp = vec3(lightPos[3][0], lightPos[3][1], lightPos[3][2]);
				color = vec3(0.5, 0.5, 1.0);

				vec2 texOff = vec2(((int)(totalTime * 15.)) % 4, ((int)(totalTime * 15.)) / 4);

				//send the matrices to the shaders
				glUniformMatrix4fv(jPurpleSphere->getUniform("P"), 1, GL_FALSE, &P[0][0]);
				glUniformMatrix4fv(jPurpleSphere->getUniform("V"), 1, GL_FALSE, &V[0][0]);
				glUniformMatrix4fv(jPurpleSphere->getUniform("M"), 1, GL_FALSE, &M[0][0]);
				glUniform3fv(jPurpleSphere->getUniform("col"), 1, &color[0]);
				glUniform3fv(jPurpleSphere->getUniform("lightPos"), 1, &lp[0]);
				glUniform2fv(jPurpleSphere->getUniform("texOff"), 1, &texOff[0]);


				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, AudioTex);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, Texture2);

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, Texture3);

				jShape->draw(jPurpleSphere, FALSE);
				jPurpleSphere->unbind();
			}
		}
	}
};
//******************************************************************************************
int main(int argc, char **argv)
{
	std::string resourceDir = "../resources"; // Where the resources are loaded from
	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	/* your main will always include a similar set up to establish your window
		and GL context, etc. */
	WindowManager * windowManager = new WindowManager();
	windowManager->init(1920, 1080);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	/* This is the code that will likely change program to program as you
		may need to initialize or set up different data and state */
	// Initialize scene.
	application->init(resourceDir);
	application->initGeom();

	thread t1(start_recording);
	// Loop until the user closes the window.
	while(! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}
	running = FALSE;
	t1.join();

	// Quit program.
	windowManager->shutdown();
	return 0;
}
