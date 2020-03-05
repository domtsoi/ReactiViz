/* Music Visualizer Code for Dominic Tsoi's Senior Project
	based on lab 5 by CPE 471 Cal Poly Z. Wood + S. Sueda
	& Ian Dunn, Christian Eckhardt
*/
#include <iostream>
#include <algorithm>
#include <glad/glad.h>
#include <time.h>
//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "camera.h"
// used for helper in perspective
#include "glm/glm.hpp"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <thread>

#include "recordAudio.h"
#include "kiss_fft.h"

//Kinect Stuff
#include "necessary_includes.h"
#include "elements/Kinect/Kinect.h"
#include "elements/Frames/Frames.h"

#define M_PI          3.141592653589793238462643383279502884L /* pi */


extern captureAudio actualAudioData;
extern int running;

//#define NOAUDIO

#define CITYSLOTS 40
#define BULDINGDIST 0.8
#define CITYSIZE (CITYSLOTS * BULDINGDIST * 2.0)
#define CITYMULSCALE 3
#define CUBE_TEXTURE_SIZE 512
#define CHANGEMODETIME 4

#define LANDSIZE 200
#define SPHERECOUNT 300
#define BILLCOUNT 200
#define SPHEREDISTANCEZ 100
#define LASERTHRESHOLD 45.0

#define TUNNELSIZE 30
#define TUNNELSPHERECOUNT 50
const float antialiasing = 1.5;

bool fullscreen = false;
const bool forceresolution = false;
#define FORCERESX 1280
#define FORCERESY 720


#define FFTW_ESTIMATEE (1U << 6)
#define FFT_MAXSIZE 500
#define FFT_REALSIZE (FFT_MAXSIZE/4)

//Kinect Attributes
#define NUM_BODIES 6
#define WIDTH 1920
#define HEIGHT 1080


int width = WIDTH;
int height = HEIGHT;

class StopWatchMicro_
{
	private:
		LARGE_INTEGER last, frequency;
	public:
		StopWatchMicro_()
		{
			QueryPerformanceFrequency(&frequency);
			QueryPerformanceCounter(&last);

		}
		long double elapse_micro()
		{
			LARGE_INTEGER now, dif;
			QueryPerformanceCounter(&now);
			dif.QuadPart = now.QuadPart - last.QuadPart;
			long double fdiff = (long double)dif.QuadPart;
			fdiff /= (long double)frequency.QuadPart;
			return fdiff * 1000000.;
		}
		long double elapse_milli()
		{
			elapse_micro() / 1000.;
		}
		void start()
		{
			QueryPerformanceCounter(&last);
		}
};

rendermodes rendermode = MODE_TUNNEL;
camera mycam;

#define MAXLASERS 10000
#define LASERTRIGGERMUL 1.0
#define LASERAGINGMUL 3.0
class laser_
{
	public:
		vec3 pos;
		vec3 col = vec3(0);
		float weight = 8;
		float age = 1.5;
		vec2 rg_col = vec2(0);
		vec3 direction = vec3(0,1,0);
		int reservour_index = -1;
		ivec2 fieldpos = ivec2(0, 0);
		void aging(float frametime)
		{
			if (age > 0) age -= frametime * LASERAGINGMUL;
			else age = 1.5;
		}
		void aging_last(float frametime)
		{
			if (age < 0.9) age -= frametime * LASERAGINGMUL*0.2;
			if (age < 1.05) age -= frametime * 0.04;
			else if (age < 1.5) age -= frametime * LASERAGINGMUL;
			if(age<0) age = 0;
		}
};

vector<laser_> laser_reservour;
vector<UINT> laser_reservour_index_stack;
vector<laser_> active_lasers;
vector<laser_> eyes_lasers;
vector<laser_> roundlaser;

void change(UINT& a, UINT& b)
{
	int c = a;
	a = b;
	b = c;
}

void changef(float& a, float& b)
{
	int c = a;
	a = b;
	b = c;
}

void shuffle_lasers(int count)
{
	int shuffle_count = count;
	if (count < 0) shuffle_count = laser_reservour_index_stack.size();
	else if (shuffle_count <= 0) return;
	for (int ii = 0; ii < shuffle_count; ii++)
	{
		int ia = rand() % shuffle_count;
		int ib = rand() % shuffle_count;
		change(laser_reservour_index_stack[ia], laser_reservour_index_stack[ib]);
	}
}

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

float delayfilter(float old, float actual, float mul)
{
	if (actual > old) return actual;
	float fold = (float)old;
	float factual = (float)actual;
	float fres = fold - (fold - factual) / mul;
	return (float)fres;
}
float delay(float old, float actual, float mul)
{
	float fold = (float)old;
	float factual = (float)actual;
	float fres = fold - (fold - factual) * mul;
	return (float)fres;
}
#define TEXELSIZE 10
void frequency_to_texture(GLuint texturebuf, int resx, int resy, float* fft_amplitudes)
{
	static float oldvals[TEXELSIZE * TEXELSIZE];
	BYTE texels[TEXELSIZE * TEXELSIZE * 4];
	//---------------------------------------------------------------------------------------
	static float fft_amplitudes_x[TEXELSIZE];
	static float fft_amplitudes_y[TEXELSIZE];
	static UINT fft_indices_x[TEXELSIZE];
	static UINT fft_indices_y[TEXELSIZE];
	static bool first = true;
	static float weights[TEXELSIZE];

	if (first)
	{
		for (int y = 0; y < TEXELSIZE; ++y)
			for (int x = 0; x < TEXELSIZE; ++x)
				oldvals[x + y * TEXELSIZE] = 0.0;
			
		for (int ii = 0; ii < TEXELSIZE; ii++)
		{
			fft_indices_x[ii] = ii;
			fft_indices_y[ii] = ii;// % 5;
			weights[ii] = 1. + pow((float)(ii / 10.0), 2)*8.;
		}
		//---------------------------------------------------------------------------------------
		for (int ii = 0; ii < TEXELSIZE; ii++)
		{
			int ia = rand() % TEXELSIZE;
			int ib = rand() % TEXELSIZE;
			change(fft_indices_x[ia], fft_indices_x[ib]);
			ia = rand() % TEXELSIZE;
			ib = rand() % TEXELSIZE;
			change(fft_indices_y[ia], fft_indices_y[ib]);
		}
		first = false;
	}
	for (int ii = 0; ii < TEXELSIZE; ii++)
	{
		int ix = fft_indices_x[ii];
		int iy = fft_indices_y[ii];
		//ix = iy = ii;
		fft_amplitudes_x[ii] = fft_amplitudes[ix];// *weights[ix];
		fft_amplitudes_y[ii] = fft_amplitudes[iy];//* weights[iy];
		fft_amplitudes_x[ii] *= 0.05;
		fft_amplitudes_y[ii] *= 0.05;
		fft_amplitudes_x[ii] = min((float)1.0, (float)fft_amplitudes_x[ii]);
		fft_amplitudes_y[ii] = min((float)1.0, (float)fft_amplitudes_y[ii]);
	}
	//---------------------------------------------------------------------------------------
	for (int y = 0; y < TEXELSIZE; ++y)
	{
		for (int x = 0; x < TEXELSIZE; ++x)
		{
			float erg = (fft_amplitudes_x[x] + fft_amplitudes_y[y]) * 200;
			oldvals[x + y * TEXELSIZE] = erg = delay(oldvals[x + y * TEXELSIZE], erg, 0.1);
			texels[x * 4 + y * TEXELSIZE * 4 + 0] = (BYTE)erg;
			texels[x * 4 + y * TEXELSIZE * 4 + 1] = (BYTE)erg;
			texels[x * 4 + y * TEXELSIZE * 4 + 2] = (BYTE)erg;
			texels[x * 4 + y * TEXELSIZE * 4 + 3] = (BYTE)erg;
		}
	}
	//----------------------------------------------------------------------
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texturebuf);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEXELSIZE, TEXELSIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, texels);
	glGenerateMipmap(GL_TEXTURE_2D);
}

class laser_vertex_
{
	public:
		vec3 position, color, ageinfo;
		laser_vertex_()
		{
			position = ageinfo = vec3(0);
			color = vec3(1);
		}
};

float lengthsq(vec3 v)
{
	return v.x* v.x + v.y * v.y + v.z * v.z;
}

bool compareInterval(laser_ a, laser_ b)
{
	return (vec4(mycam.V * vec4(a.pos, 1)).z < vec4(mycam.V * vec4(b.pos, 1)).z);
	//return (lengthsq(a.pos+ mycam.pos) < lengthsq(b.pos+ mycam.pos));
}

bool fft(float* amplitude_on_frequency, int& length)
{
	int N = pow(2, 10);
	BYTE data[MAXS];
	int size = 0;
	actualAudioData.readAudio(data, size);
	length = size / 8;

	if (size == 0)
		return false;

	double* samples = new double[length];
	for (int ii = 0; ii < length; ii++)
	{
		float* f = (float*)& data[ii * 8];
		samples[ii] = (double)(*f);
	}

	kiss_fft_cpx* cx_in = new kiss_fft_cpx[length];
	kiss_fft_cpx* cx_out = new kiss_fft_cpx[length];
	kiss_fft_cfg cfg = kiss_fft_alloc(length, 0, 0, 0);
	for (int i = 0; i < length; ++i)
	{
		cx_in[i].r = samples[i];
		cx_in[i].i = 0;
	}
	delete[] samples;
	
	kiss_fft(cfg, cx_in, cx_out);

	float amplitude_on_frequency_old[FFT_MAXSIZE];
	for (int i = 0; i < length / 2 && i < FFT_MAXSIZE; ++i)
		amplitude_on_frequency_old[i] = amplitude_on_frequency[i];

	for (int i = 0; i < length / 2 && i < FFT_MAXSIZE; ++i)
		amplitude_on_frequency[i] = sqrt(pow(cx_out[i].i, 2.) + pow(cx_out[i].r, 2.));


	delete[] cx_in;
	delete[] cx_out;
	
	//that looks better, decomment for no filtering: +++++++++++++++++++
	for (int i = 0; i < length / 2 && i < FFT_MAXSIZE; ++i)
	{
		float diff = amplitude_on_frequency_old[i] - amplitude_on_frequency[i];
		float attack_factor = 0.1;//for going down
		if (amplitude_on_frequency_old[i] < amplitude_on_frequency[i])
			attack_factor = 0.85; //for going up
		diff *= attack_factor;
		amplitude_on_frequency[i] = amplitude_on_frequency_old[i] - diff;
	//std:cout << "Current amplitude: " << amplitude_on_frequency[i] << "@ frequency" << i << endl;
	}
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	length /= 2;
	free(cfg);

	return true;
}

double get_last_elapsed_time()
{
	static double lasttime = glfwGetTime();
	double actualtime = glfwGetTime();
	double difference = actualtime - lasttime;
	lasttime = actualtime;
	return difference;
}

using namespace std;
using namespace glm;
float frand()
{
	return (float)rand() / 32768.0;
}

class cityfieldelement_
{
	public: float height = 0; bool laseractive = false;
};

class Application : public EventCallbacks
{
	public:
		bool beattrigger = false;
		bool lasertrigger = false;
		bool kinect_music = false;
		int initialized_VB_laser_count = 0;
		float amplitude_on_frequency[FFT_MAXSIZE];
		float amplitude_on_frequency_10steps[10];
		float global_music_influence = 0;

		float z_schwenk = 0;
		mat4 M_wobble = mat4(1), M_facetrinity = mat4(1);

		WindowManager* windowManager = nullptr;

		// Our shader program
		std::shared_ptr<Program> prog, progs, prog2, progsky, progg, prog_laser, prog_blur, progland, progland2, prog_bloom, progspheres, progspheres_tunnel, prog_face, prog_roundlaser, prog_lasereyes, progtunnel2, progtunnel, prog_tv;
		std::shared_ptr<Kinect> kinect = make_shared<Kinect>(NUM_BODIES, WIDTH, HEIGHT); 
		bool toogle_view = false;
		// Shape to be used (from obj file)
		shared_ptr<Shape> shape, sphere, armor,rects, sphere_land, sphere_tunnel, face,eyes, tv;
		cityfieldelement_ cityfield[CITYSLOTS * 2 + 1][CITYSLOTS * 2 + 1];
		//camera


		//texture for sim
		GLuint TextureEarth, TexNoise, FBObloommask, TexGod, TextureSky, TextureLandBG, TextureLandBGgod, TexLaser, g_cubeTexture, TexSuitMask, TexSuit, TexLensdirt, TexLaserMask, TexTV;
		GLuint TextureF[6], FBOcolor, fb, FBO_blur, depth_rb, depth_blur , FBOviewpos, FBOworldnormal, FBOworldpos, FBOgodrays, FBOcolor_no_ssaa[2], FBOcolor_blurmap, FBOcolor_bloommap,FBOtv;
		GLuint Cube_framebuffer, Cube_depthbuffer, VBLasersPos, VBLasersCol, VAOLasers,VAOtunnellaser, VAOland, IBland, IBlandfull, VBLasersDir, VAOtunnel, IBtunnel, IBtunnelfull,IBtunnellaser;
		GLuint IBlandsize, IBtunnelsize, IBlandfullsize, IBtunnelfullsize, TexAudio, IBtunnellasersize, TexTunnelLaser, VB_roundlaser_frequ;
		GLuint VertexArrayIDBox, VertexBufferIDBox, VertexBufferTex;

		// Contains vertex information for OpenGL
		GLuint VertexArrayID;

		// Data necessary to give our triangle to OpenGL
		GLuint VertexBufferID;

		//texturetest stuff
		shared_ptr<Shape> texturetest_skysphere;
		// Our shader program
		std::shared_ptr<Program> ptexturetest1, ptexturetestsky, ptexturetestpostproc;
		GLuint texturetestVertexArrayID, texturetestVertexArrayIDScreen;
		GLuint texturetestVertexBufferID, texturetestVertexBufferTexScreen, texturetestVertexBufferIDScreen, texturetestVertexNormDBox, texturetestVertexTexBox, texturetestIndexBufferIDBox, texturetestInstanceBuffer;
		//framebufferstuff
		GLuint texturetestfb, texturetestdepth_fb, texturetestFBOtex; //Look at FBOtex
		//texture data
		GLuint texturetestTexture;
		GLuint texturetestTexture2;
		GLuint TVtex;
		float kinect_depth = 0;
	void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if (key == GLFW_KEY_B && action == GLFW_RELEASE)
		{
			beattrigger = true;
			lasertrigger = true;
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
			z_schwenk = -1;
		}
		if (key == GLFW_KEY_A && action == GLFW_RELEASE)
		{
			mycam.a = 0;
			z_schwenk = 0;
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS)
		{
			mycam.d = 1;
			z_schwenk = 1;
		}
		if (key == GLFW_KEY_D && action == GLFW_RELEASE)
		{
			mycam.d = 0;
			z_schwenk = 0;
		}
		if (key == GLFW_KEY_F && action == GLFW_RELEASE)
		{
			fullscreen = !fullscreen;
		}
		if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
		{
			mycam.toggle();
		}
		if (key == GLFW_KEY_P && action == GLFW_RELEASE)
		{
			mycam.toggle_auto = !mycam.toggle_auto;
		}
		if (key == GLFW_KEY_ENTER && action == GLFW_RELEASE)
		{
			if (mycam.rot.z < 1)
				mycam.rot.z = 3.1415926;
			else
				mycam.rot.z = 0;
		}
		if (key == GLFW_KEY_K && action == GLFW_RELEASE)
		{
			if (kinect_music)
			{
				kinect_music = false;
			}
			else
			{
				kinect_music = true;
			}
		}
		if (key == GLFW_KEY_F1)
		{
			rendermode = MODE_CITYFWD;
			mycam.reset(rendermode);
			mycam.toggle_auto = true;
			initialized_VB_laser_count = 0;
		}
		if (key == GLFW_KEY_F2)
		{
			rendermode = MODE_CITYSTATIC;				
			mycam.reset(rendermode);
			mycam.rot.y = -0.4;
			mycam.oldrot.y = -0.4;
			initialized_VB_laser_count = 0;
		}
		if (key == GLFW_KEY_F3)
		{
			rendermode = MODE_LANDFWD;
			mycam.reset(rendermode);
			mycam.rot.x = 0.1;
			mycam.oldrot.x = 0.1;
			initialized_VB_laser_count = 0;
		}
		if (key == GLFW_KEY_F4)
		{
			rendermode = MODE_TUNNEL;
			mycam.reset(rendermode);
			initialized_VB_laser_count = 0;
		}			
		if (key == GLFW_KEY_F5)
		{
			rendermode = MODE_BODYSENSE_STATIC;
			mycam.reset(rendermode);
		}
		if (key == GLFW_KEY_F6)
		{
			rendermode = MODE_TEXTURE_TEST;
			mycam.reset(rendermode);
		}
		if (key == GLFW_KEY_UP && action == GLFW_RELEASE)
		{
			kinect_depth += 0.02;
			cout << "UP PRESSED, kinectDepth: " << kinect_depth << endl;
			
		}
		if (key == GLFW_KEY_DOWN && action == GLFW_RELEASE)
		{
			kinect_depth -= 0.02;
			cout << "DOWN PRESSED, kinectDepth: " << kinect_depth << endl;
			
		}
	}

	void CreateCubeTexture()
	{
		glGenTextures(1, &g_cubeTexture);
		glBindTexture(GL_TEXTURE_CUBE_MAP, g_cubeTexture);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		std::vector<GLubyte> testData(CUBE_TEXTURE_SIZE * CUBE_TEXTURE_SIZE * 256, 128);
		std::vector<GLubyte> xData(CUBE_TEXTURE_SIZE * CUBE_TEXTURE_SIZE * 256, 255);

		for (int loop = 0; loop < 6; ++loop)
		{
			if (loop)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + loop, 0, GL_RGBA8,
								CUBE_TEXTURE_SIZE, CUBE_TEXTURE_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, &testData[0]);
			}
			else
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + loop, 0, GL_RGBA8,
								CUBE_TEXTURE_SIZE, CUBE_TEXTURE_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, &xData[0]);
			}
		}

		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

	void mouseCallback(GLFWwindow * window, int button, int action, int mods)
	{
		double posX, posY;

		if (action == GLFW_PRESS)
		{
			glfwGetCursorPos(window, &posX, &posY);
			cout << "Pos X " << posX << " Pos Y " << posY << endl;
		}
	}

	void resizeCallback(GLFWwindow * window, int width, int height)
	{
		//remove all window size associated suff:

		glDeleteFramebuffers(1, &fb);
		glDeleteFramebuffers(1, &FBO_blur);

		glDeleteTextures(1, &FBOcolor);
		glDeleteTextures(1, &FBOviewpos);
		glDeleteTextures(1, &FBOworldpos);
		glDeleteTextures(1, &FBOgodrays);
		glDeleteTextures(1, &FBOworldnormal);
		glDeleteTextures(1, &FBObloommask);
		glDeleteTextures(1, &FBOcolor_no_ssaa[0]);
		glDeleteTextures(1, &FBOcolor_no_ssaa[1]);
		glDeleteTextures(1, &FBOcolor_blurmap);

		glDeleteRenderbuffers(1, &depth_rb);
		glDeleteRenderbuffers(1, &depth_blur);

		generate_flexible_framebuffers();

		get_resolution(&width, &height);
		glViewport(0, 0, width, height);
	}

	void generate_flexible_framebuffers()
	{
		int width, height;
		get_resolution(&width, &height);

		//create frame buffer
		glGenFramebuffers(1, &fb);
		glActiveTexture(GL_TEXTURE0);
		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		//RGBA8 2D texture, 24 bit depth texture, 256x256
		FBOcolor = generate_texture2D(GL_RGBA8, width * antialiasing, height * antialiasing, GL_RGBA, GL_UNSIGNED_BYTE, NULL, GL_CLAMP_TO_BORDER, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
		FBOviewpos = generate_texture2D(GL_RGBA32F, width * antialiasing, height * antialiasing, GL_RGBA, GL_FLOAT, NULL, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
		FBOworldpos = generate_texture2D(GL_RGBA32F, width * antialiasing, height * antialiasing, GL_RGBA, GL_FLOAT, NULL, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
		FBOgodrays = generate_texture2D(GL_RGBA8, width * antialiasing, height * antialiasing, GL_RGBA, GL_UNSIGNED_BYTE, NULL, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
		FBObloommask = generate_texture2D(GL_RGBA8, width * antialiasing, height * antialiasing, GL_RGBA, GL_UNSIGNED_BYTE, NULL, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
		FBOworldnormal = generate_texture2D(GL_RGBA32F, width * antialiasing, height * antialiasing, GL_RGBA, GL_FLOAT, NULL, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
		//Attach 2D texture to this FBO
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOcolor, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, FBOviewpos, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, FBOworldpos, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, FBOworldnormal, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, FBOgodrays, 0); 
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, GL_TEXTURE_2D, FBObloommask, 0);
		//create depth buffer
		glGenRenderbuffers(1, &depth_rb);
		glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width * antialiasing, height * antialiasing);
		//Attach depth buffer to FBO
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);
		//-------------------------
		glBindFramebuffer(GL_FRAMEBUFFER, 0);


		//create frame buffer for two step blur
		glGenFramebuffers(1, &FBO_blur);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO_blur);
		FBOcolor_no_ssaa[0] = generate_texture2D(GL_RGBA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, NULL, GL_CLAMP_TO_BORDER, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
		FBOcolor_no_ssaa[1] = generate_texture2D(GL_RGBA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, NULL, GL_CLAMP_TO_BORDER, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
		FBOcolor_blurmap = generate_texture2D(GL_RGBA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, NULL, GL_CLAMP_TO_BORDER, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
		FBOcolor_bloommap = generate_texture2D(GL_RGBA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, NULL, GL_CLAMP_TO_BORDER, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOcolor_no_ssaa[0], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, FBOcolor_blurmap, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, FBOcolor_bloommap, 0);

		glGenRenderbuffers(1, &depth_blur);
		glBindRenderbuffer(GL_RENDERBUFFER, depth_blur);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_blur);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//*******************************************************************************************************
	}

	void texturetest_init(const std::string& resourceDirectory)
	{
		// Initialize the GLSL program.
		ptexturetest1 = std::make_shared<Program>();
		ptexturetest1->setVerbose(true);
		ptexturetest1->setShaderNames(resourceDirectory + "/shader_vertex.glsl", resourceDirectory + "/shader_fragment.glsl");
		if (!ptexturetest1->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		ptexturetest1->addUniform("P");
		ptexturetest1->addUniform("V");
		ptexturetest1->addUniform("M");
		ptexturetest1->addUniform("campos");
		ptexturetest1->addUniform("music_influence");
		ptexturetest1->addAttribute("vertPos");
		ptexturetest1->addAttribute("vertNor");
		ptexturetest1->addAttribute("vertTex");
		ptexturetest1->addAttribute("InstancePos");

		ptexturetestsky = std::make_shared<Program>();
		ptexturetestsky->setVerbose(true);
		ptexturetestsky->setShaderNames(resourceDirectory + "/skyvertex.glsl", resourceDirectory + "/skyfrag.glsl");
		if (!ptexturetestsky->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		ptexturetestsky->addUniform("P");
		ptexturetestsky->addUniform("V");
		ptexturetestsky->addUniform("M");
		ptexturetestsky->addUniform("campos");
		ptexturetestsky->addAttribute("vertPos");
		ptexturetestsky->addAttribute("vertNor");
		ptexturetestsky->addAttribute("vertTex");

		//program for the postprocessing
		ptexturetestpostproc = std::make_shared<Program>();
		ptexturetestpostproc->setVerbose(true);
		ptexturetestpostproc->setShaderNames(resourceDirectory + "/postproc_vertex.glsl", resourceDirectory + "/postproc_fragment.glsl");
		if (!ptexturetestpostproc->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		ptexturetestpostproc->addAttribute("vertPos");
		ptexturetestpostproc->addAttribute("vertTex");

	}

	void init(const std::string & resourceDirectory)
	{
		mycam.reset(rendermode);

		GLSL::checkVersion();

		//Initialize kinect
		kinect->init();

		//Initialize texture test scene
		texturetest_init(resourceDirectory);

		// Set background color.
		glClearColor(0.12f, 0.34f, 0.56f, 1.0f);

		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		//culling:
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);

		//transparency
		glEnable(GL_BLEND);
		//next function defines how to mix the background color with the transparent pixel in the foreground. 
		//This is the standard:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Initialize the GLSL program.
		prog = make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/instance_vert.glsl", resourceDirectory + "/simple_frag.glsl");
		if (!prog->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog->addUniform("freq_ampl");
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("reflection_on");
		prog->addUniform("render_lines");
		prog->addUniform("campos");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");
		prog->addAttribute("vertTex");

		progspheres = make_shared<Program>();
		progspheres->setVerbose(true);
		progspheres->setShaderNames(resourceDirectory + "/instance_sphere_vert.glsl", resourceDirectory + "/sphere_frag.glsl");
		if (!progspheres->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		progspheres->addUniform("freq_ampl");
		progspheres->addUniform("P");
		progspheres->addUniform("V");
		progspheres->addUniform("M");
		progspheres->addUniform("reflection_on");
		progspheres->addUniform("render_lines");
		progspheres->addUniform("campos");
		progspheres->addAttribute("vertPos");
		progspheres->addAttribute("vertNor");
		progspheres->addAttribute("vertTex"); 
		progspheres->addUniform("colormode");

		progspheres_tunnel = make_shared<Program>();
		progspheres_tunnel->setVerbose(true);
		progspheres_tunnel->setShaderNames(resourceDirectory + "/instance_sphere_tunnel_vert.glsl", resourceDirectory + "/sphere_tunnel_frag.glsl");
		if (!progspheres_tunnel->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		progspheres_tunnel->addUniform("freq_ampl");
		progspheres_tunnel->addUniform("P");
		progspheres_tunnel->addUniform("V");
		progspheres_tunnel->addUniform("M");
		progspheres_tunnel->addUniform("reflection_on");
		progspheres_tunnel->addUniform("render_lines");
		progspheres_tunnel->addUniform("campos");
		progspheres_tunnel->addAttribute("vertPos");
		progspheres_tunnel->addAttribute("vertNor");
		progspheres_tunnel->addAttribute("vertTex");		

		progs = make_shared<Program>();
		progs->setVerbose(true);
		progs->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/suit_frag.glsl");
		if (!progs->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}

		progs->addUniform("P");
		progs->addUniform("V");
		progs->addUniform("M");
		progs->addUniform("reflection_on");
		progs->addUniform("campos");
		progs->addAttribute("vertPos");
		progs->addAttribute("vertNor");
		progs->addAttribute("vertTex");
		progs->addUniform("render_lines");

		prog_face = make_shared<Program>();
		prog_face->setVerbose(true);
		prog_face->setShaderNames(resourceDirectory + "/face_vert.glsl", resourceDirectory + "/face_frag.glsl");
		if (!prog_face->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog_face->addUniform("P");
		prog_face->addUniform("V"); 
		prog_face->addUniform("M");
		prog_face->addUniform("extcolor");
		prog_face->addAttribute("vertPos");
		prog_face->addAttribute("vertNor");
		prog_face->addAttribute("vertTex");

		prog_roundlaser = make_shared<Program>();
		prog_roundlaser->setVerbose(true);
		prog_roundlaser->setShaderNames(resourceDirectory + "/rlaser_vert.glsl", resourceDirectory + "/rlaser_frag.glsl");
		if (!prog_roundlaser->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog_roundlaser->addUniform("P");
		prog_roundlaser->addUniform("V");
		prog_roundlaser->addUniform("M");
		prog_roundlaser->addUniform("extcolor");
		prog_roundlaser->addAttribute("vertPos");
		prog_roundlaser->addAttribute("vertTex");
		prog_roundlaser->addAttribute("vertFreq");

		prog2 = make_shared<Program>();
		prog2->setVerbose(true);
		prog2->setShaderNames(resourceDirectory + "/vert.glsl", resourceDirectory + "/frag_nolight.glsl");
		if (!prog2->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}

		prog2->addUniform("P");
		prog2->addUniform("V");
		prog2->addUniform("M");
		prog2->addUniform("speed");
		prog2->addAttribute("vertPos");
		prog2->addAttribute("vertTex");
		prog2->addUniform("Pcam");
		prog2->addUniform("Vcam");
		prog2->addUniform("windows_size");

		prog_blur = make_shared<Program>();
		prog_blur->setVerbose(true);
		prog_blur->setShaderNames(resourceDirectory + "/vert.glsl", resourceDirectory + "/blur_frag.glsl");
		if (!prog_blur->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog_blur->addUniform("P");
		prog_blur->addUniform("V");
		prog_blur->addUniform("M");
		prog_blur->addAttribute("vertPos");
		prog_blur->addAttribute("vertTex");
		prog_blur->addUniform("speed");
		prog_blur->addUniform("Pcam");
		prog_blur->addUniform("Vcam");
		prog_blur->addUniform("stepno");
		prog_blur->addUniform("bluractive");
		prog_blur->addUniform("windows_size");

		prog_bloom = make_shared<Program>();
		prog_bloom->setVerbose(true);
		prog_bloom->setShaderNames(resourceDirectory + "/vert.glsl", resourceDirectory + "/bloom_frag.glsl");
		if (!prog_bloom->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog_bloom->addUniform("P");
		prog_bloom->addUniform("V");
		prog_bloom->addUniform("M");
		prog_bloom->addAttribute("vertPos");
		prog_bloom->addAttribute("vertTex");
		prog_bloom->addUniform("speed");
		prog_bloom->addUniform("Pcam");
		prog_bloom->addUniform("Vcam");
		prog_bloom->addUniform("stepno");
		prog_bloom->addUniform("bluractive");
		prog_bloom->addUniform("windows_size");

		progsky = make_shared<Program>();
		progsky->setVerbose(true);
		progsky->setShaderNames(resourceDirectory + "/vert.glsl", resourceDirectory + "/sky_frag.glsl");
		if (!progsky->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		progsky->addUniform("P");
		progsky->addUniform("V");
		progsky->addUniform("M");
		progsky->addAttribute("vertPos");
		progsky->addAttribute("vertTex");

		progland = make_shared<Program>();
		progland->setVerbose(true);
		progland->setShaderNames(resourceDirectory + "/land_vert.glsl", resourceDirectory + "/land_frag.glsl");
		if (!progland->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		progland->addUniform("P");
		progland->addUniform("V");
		progland->addUniform("M");
		progland->addUniform("offset");
		progland->addUniform("screen_offset");
		progland->addAttribute("vertPos");
		progland->addAttribute("vertTex");

		progland2= make_shared<Program>();
		progland2->setVerbose(true);
		progland2->setShaderNames(resourceDirectory + "/land_vert.glsl", resourceDirectory + "/land_fragfull.glsl");
		if (!progland2->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		progland2->addUniform("P");
		progland2->addUniform("V");
		progland2->addUniform("M");
		progland2->addUniform("offset");
		progland2->addUniform("screen_offset");
		progland2->addAttribute("vertPos");
		progland2->addAttribute("vertTex");

		progtunnel = make_shared<Program>();
		progtunnel->setVerbose(true);
		progtunnel->setShaderNames(resourceDirectory + "/tunnel_vert.glsl", resourceDirectory + "/tunnel_frag.glsl");
		if (!progtunnel->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		progtunnel->addUniform("P");
		progtunnel->addUniform("V");
		progtunnel->addUniform("M");
		progtunnel->addUniform("offset");
		progtunnel->addUniform("screen_offset");
		progtunnel->addAttribute("vertPos");
		progtunnel->addAttribute("vertTex");

		progtunnel2 = make_shared<Program>();
		progtunnel2->setVerbose(true);
		progtunnel2->setShaderNames(resourceDirectory + "/tunnel_vertfull.glsl", resourceDirectory + "/tunnel_fragfull.glsl", resourceDirectory + "/tunnel_geometry.glsl");
		if (!progtunnel2->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		progtunnel2->addUniform("P");
		progtunnel2->addUniform("V");
		progtunnel2->addUniform("M");
		progtunnel2->addUniform("roundlaserpos");
		progtunnel2->addUniform("roundlasercol");
		progtunnel2->addUniform("roundlasercount");
			
		progtunnel2->addUniform("offset");
		progtunnel2->addUniform("campos");
		progtunnel2->addUniform("screen_offset");
		progtunnel2->addAttribute("vertPos");
		progtunnel2->addAttribute("vertTex");
			

		progg = make_shared<Program>();
		progg->setVerbose(true);
		progg->setShaderNames(resourceDirectory + "/vert.glsl", resourceDirectory + "/ground_frag.glsl");
		if (!progg->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		progg->addUniform("P");
		progg->addUniform("V");
		progg->addUniform("M");
		progg->addAttribute("vertPos");
		progg->addAttribute("vertTex");


		prog_laser = make_shared<Program>();
		prog_laser->setVerbose(true);
		prog_laser->setShaderNames(resourceDirectory + "/vert_laser.glsl", resourceDirectory + "/laser_frag.glsl", resourceDirectory + "/geometry_laser.glsl");
		if (!prog_laser->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog_laser->addUniform("P");
		prog_laser->addUniform("V");
		prog_laser->addUniform("M");
		prog_laser->addAttribute("vertPos");
		prog_laser->addAttribute("vertCol");
		prog_laser->addAttribute("vertDir");

		prog_lasereyes = make_shared<Program>();
		prog_lasereyes->setVerbose(true);
		prog_lasereyes->setShaderNames(resourceDirectory + "/vert_laser.glsl", resourceDirectory + "/laser_frag.glsl", resourceDirectory + "/geometry_lasereyes.glsl");
		if (!prog_lasereyes->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog_lasereyes->addUniform("P");
		prog_lasereyes->addUniform("V");
		prog_lasereyes->addUniform("M");	
		prog_lasereyes->addAttribute("vertPos");
		prog_lasereyes->addAttribute("vertCol");
		prog_lasereyes->addAttribute("vertDir");

		//Initialize kinect
		kinect->init();

		prog_tv = make_shared<Program>();
		prog_tv->setVerbose(true);
		prog_tv->setShaderNames(resourceDirectory + "/tv_vertex.glsl", resourceDirectory + "/tv_fragment");
		if (!prog_tv->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog_tv->addUniform("P");
		prog_tv->addUniform("V");
		prog_tv->addUniform("M");
		prog_tv->addUniform("campos");
		prog_tv->addAttribute("vertPos");
		prog_tv->addAttribute("vertNor");
		prog_tv->addAttribute("vertTex");
		prog_tv->addAttribute("InstancePos");
}

	void prepare_to_render()
	{
		kinect->get_depth();
		kinect->get_color();
		kinect->update_bodies();
	}

	void render()
	{
		double frametime = get_last_elapsed_time();
		get_resolution(&width, &height);
		float aspect = width / (float)height;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//Change Stuff here to edit the viewport
		glViewport(0, 0, width, height);
		glClearColor(.8, .8, 1, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		if (kinect_music)
		{
			kinect->draw_bodies(global_music_influence,TVtex, kinect_depth);
		}
		else
		{
			kinect->draw_bodies();
		}
	}

	GLuint generate_texture2D(GLushort colortype, int width, int height, GLushort colororder, GLushort datatype, BYTE * data, GLushort wrap, GLushort minfilter, GLushort magfilter)
	{
		GLuint textureID;
		//RGBA8 2D texture, 24 bit depth texture, 256x256
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magfilter);
		glTexImage2D(GL_TEXTURE_2D, 0, colortype, width, height, 0, colororder, datatype, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		return textureID;
	}
	//**************************************************************************************************************************************************
//**************************************************************************************************************************************************
	void init_tunnel(const std::string& resourceDirectory)
	{
		vector<float> instancepossphere;

		GLuint VB;
		glGenVertexArrays(1, &VAOtunnel);
		glBindVertexArray(VAOtunnel);
			
		vector<vec3> pos;
		vector<vec2> textunnel;
		//front to back
		float w = 0;
		float ww = 0.1;
		mat4 Rz = rotate(mat4(1), w, vec3(0, 0, 1));
		vec4 xypos = Rz * vec4(1, 0, 0, 1);
		for (int zz = 0; zz < TUNNELSIZE * 10; zz++)
		{
			Rz = rotate(mat4(1), w, vec3(0, 0, 1));				
			w += ww;
			float rand_firstvec = 0.0;
			for (int xx = 0; xx <= TUNNELSIZE; xx++)
			{
					
				float xarg = ((float)(xx) / (float)TUNNELSIZE) * 3.1415926 * 2.0;
				float yarg = xarg + 3.1415926 * 0.5;
				if (xx == TUNNELSIZE || xx == 0) 
				{ 
					xarg = 0.0; yarg = 3.1415926 * 0.5; 
				}
				float randvec = frand() * 0.1 + 0.9;
					
				//randvec = 1;
				randvec *= 20.0;
				if (xx == 0)
				{
					rand_firstvec = randvec;
				}
				else if (xx == TUNNELSIZE)
				{
					randvec = rand_firstvec;
				}
				vec4 xypos = Rz * vec4(sin(xarg) * randvec, cos(xarg) * randvec, -zz * 5, 1);
				pos.push_back(vec3(xypos));
				if (xx == TUNNELSIZE)
				{
					textunnel.push_back(vec2(0.0, (float)zz / (float)(TUNNELSIZE * 10)));
				}
				else
				{
					textunnel.push_back(vec2((float)xx / (float)TUNNELSIZE, (float)zz / (float)(TUNNELSIZE * 10)));
				}
			}
		}
		glGenBuffers(1, &VB);
		glBindBuffer(GL_ARRAY_BUFFER, VB);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * pos.size(), pos.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glGenBuffers(1, &VB);
		glBindBuffer(GL_ARRAY_BUFFER, VB);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * textunnel.size(), textunnel.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// index buffer lines
		glGenBuffers(1, &IBtunnel);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBtunnel);
		vector<int> elements;
		float z_tunnelsize = TUNNELSIZE * 10;
		for (int zz = 0; zz < z_tunnelsize; zz++)
		{
			
			//startoffset = 0;
			for (int xx = 0; xx < TUNNELSIZE ; xx++)
			{
				if ( zz == z_tunnelsize - 1) continue;
				elements.push_back(xx + zz * (TUNNELSIZE+1));
				elements.push_back(xx + zz * (TUNNELSIZE + 1) + 1);
				elements.push_back(xx + zz * (TUNNELSIZE + 1));
				elements.push_back(xx + zz * (TUNNELSIZE + 1) + (TUNNELSIZE + 1));
				}
			}
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * elements.size(), elements.data(), GL_STATIC_DRAW);
		IBtunnelsize = elements.size();

		glGenBuffers(1, &IBtunnelfull);
			
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBtunnelfull);
		elements.clear();
		for (int zz = 0; zz < z_tunnelsize; zz++)
		{	
			//startoffset = 0;
			for (int xx = 0; xx < TUNNELSIZE; xx++)
			{
				if (zz == z_tunnelsize - 1) continue;
				elements.push_back(xx + zz * (TUNNELSIZE + 1));
				elements.push_back(xx + zz * (TUNNELSIZE + 1) + (TUNNELSIZE + 1));
				elements.push_back(xx + zz * (TUNNELSIZE + 1) + (TUNNELSIZE + 1) + 1);
				elements.push_back(xx + zz * (TUNNELSIZE + 1));
				elements.push_back(xx + zz * (TUNNELSIZE + 1) + (TUNNELSIZE + 1) + 1);
				elements.push_back(xx + zz * (TUNNELSIZE + 1) + 1);
			}
		}
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * elements.size(), elements.data(), GL_STATIC_DRAW);
		IBtunnelfullsize = elements.size();

		glBindVertexArray(0);

		sphere_tunnel = make_shared<Shape>();
		sphere_tunnel->loadMesh(resourceDirectory + "/sphere.obj");
		sphere_tunnel->resize();	

		for (int ii = 0; ii < TUNNELSPHERECOUNT; ii++)
		{
			float w = frand()*M_PI*2.0;
			mat4 Rz = rotate(mat4(1), w, vec3(0, 0, 1));
			vec4 xypos = Rz * vec4(1, 0, 0, 1);
			xypos *= pow(frand(), 0.3); //radius
			xypos *= 0.5; //radius
			xypos += vec4(0.5, 0.5, 0.0,0.0);
			vec3 pos = vec3(
				xypos.x * TUNNELSIZE - TUNNELSIZE / 2,
				xypos.y * TUNNELSIZE - TUNNELSIZE / 2,
				-frand() * TUNNELSIZE * 10
			);
			pos.x = floor(pos.x);
			pos.z = floor(pos.z);
			instancepossphere.push_back(pos.x);
			instancepossphere.push_back(pos.y);
			instancepossphere.push_back(pos.z); //rotation
			instancepossphere.push_back(frand());//size
			
			instancepossphere.push_back(frand()*0.7 +0.3);		instancepossphere.push_back(frand() * 0.7 + 0.3);		instancepossphere.push_back(frand() * 0.7 + 0.3);		instancepossphere.push_back(frand()*2);
			instancepossphere.push_back(frand() * 0.7 + 0.3);		instancepossphere.push_back(frand() * 0.7 + 0.3);		instancepossphere.push_back(frand() * 0.7 + 0.3);		instancepossphere.push_back(frand());
			instancepossphere.push_back(0);		instancepossphere.push_back(0);		instancepossphere.push_back(0);		instancepossphere.push_back(0);
				
		}
		sphere_tunnel->init(0, true, 3, &instancepossphere, sizeof(mat4));

		//rundlaser
		vector<float> frequlas;
		vector<vec3> poslas;
		vector<vec2> texlas;
		vector<GLuint> indices;
		ww = 1. / (float)FFT_REALSIZE;
		float thickness = 0.8;
		for (int i = 0; i < FFT_REALSIZE; i++)
		{
			poslas.push_back(vec3(sin((float)i * ww * M_PI * 2.) * thickness, cos((float)i * ww * M_PI * 2.) * thickness, 0));
			poslas.push_back(vec3(sin((float)i * ww * M_PI * 2.), cos((float)i * ww * M_PI * 2.), 0));
			texlas.push_back(vec2((float)i * ww, 0.));
			texlas.push_back(vec2((float)i * ww, 1.));
			frequlas.push_back(0.0);
			frequlas.push_back(0.0);
		}
		poslas.push_back(poslas[0]);
		poslas.push_back(poslas[1]);
		texlas.push_back(vec2(1., 0.));
		texlas.push_back(vec2(1., 1.));
		int cc = 0;

		for (int i = 0; i < FFT_REALSIZE; i++)
		{
			indices.push_back(cc + 1);
			indices.push_back(cc + 0);
			indices.push_back(cc + 2);
			indices.push_back(cc + 1);
			indices.push_back(cc + 2);
			indices.push_back(cc + 3);
			cc += 2;
		}

		glGenVertexArrays(1, &VAOtunnellaser);
		glBindVertexArray(VAOtunnellaser);
		glGenBuffers(1, &VB);
		glBindBuffer(GL_ARRAY_BUFFER, VB);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)* poslas.size(), poslas.data(), GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			
		glGenBuffers(1, &VB);
			
		glBindBuffer(GL_ARRAY_BUFFER, VB);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec2)* texlas.size(), texlas.data(), GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glGenBuffers(1, &VB_roundlaser_frequ);
		glBindBuffer(GL_ARRAY_BUFFER, VB_roundlaser_frequ);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)* frequlas.size(), frequlas.data(), GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
			
		glGenBuffers(1, &IBtunnellaser);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBtunnellaser);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)* indices.size(), indices.data(), GL_STATIC_DRAW);
		IBtunnellasersize = indices.size();
		glBindVertexArray(0);
		glEnableVertexAttribArray(0);
			
	}

	//**************************************************************************************************************************************************

	void init_land()
	{
		GLuint VB;
		glGenVertexArrays(1, &VAOland);
		glBindVertexArray(VAOland);			
		glGenBuffers(1, &VB);
		glBindBuffer(GL_ARRAY_BUFFER, VB);
		vector<vec3> pos;
		//front to back
			
		for (int zz = 0; zz < LANDSIZE; zz++)
			for (int xx = 0; xx < LANDSIZE; xx++)
			{
				pos.push_back(vec3(xx- LANDSIZE/2, 0, -zz)*2.0f);
			}
			
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * pos.size(), pos.data(), GL_STATIC_DRAW);
		
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			
		// index buffer lines
		glGenBuffers(1, &IBland);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBland);
		vector<int> elements;
		for (int zz = 0; zz < LANDSIZE; zz++)
		{
			int startoffset = max(0, (LANDSIZE - 5) - zz*2)/2;
			//startoffset = 0;
			for (int xx = startoffset; xx < LANDSIZE- startoffset; xx++)
			{					
				if (xx == LANDSIZE - 1 || zz == LANDSIZE - 1) continue;
				elements.push_back(xx + zz* LANDSIZE);
				elements.push_back(xx + zz * LANDSIZE + 1);
				elements.push_back(xx + zz * LANDSIZE);
				elements.push_back(xx + zz * LANDSIZE + LANDSIZE);
			}
		}
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * elements.size(), elements.data(), GL_STATIC_DRAW);
			
		glGenBuffers(1, &IBlandfull);
		IBlandsize = elements.size();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBlandfull);
		elements.clear();
		for (int zz = 0; zz < LANDSIZE; zz++)
		{
			int startoffset = max(0, (LANDSIZE - 3) - zz * 2) / 2;
			//startoffset = 0;
			for (int xx = startoffset; xx < LANDSIZE - startoffset; xx++)
			{
				if (xx == LANDSIZE - 1 || zz == LANDSIZE - 1) continue;
				elements.push_back(xx + zz * LANDSIZE);
				elements.push_back(xx + zz * LANDSIZE + LANDSIZE);
				elements.push_back(xx + zz * LANDSIZE + LANDSIZE + 1);
				elements.push_back(xx + zz * LANDSIZE);
				elements.push_back(xx + zz * LANDSIZE + LANDSIZE + 1);
				elements.push_back(xx + zz * LANDSIZE + 1);					
			}
		}
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * elements.size(), elements.data(), GL_STATIC_DRAW);
		IBlandfullsize = elements.size();



		glBindVertexArray(0);
	}
	//Dom TV Initialization
	void initTV(const std::string& resourceDirectory)
	{
		tv = make_shared<Shape>();
		tv->loadMesh(resourceDirectory + "/OldTV2.obj");
		tv->resize();
		tv->init();
	}


	void texturetest_initGeom()
	{
		string resourceDirectory = "../resources";
		// Initialize mesh.
		texturetest_skysphere = make_shared<Shape>();
		texturetest_skysphere->loadMesh(resourceDirectory + "/sphere.obj");
		texturetest_skysphere->resize();
		texturetest_skysphere->init();

		//screen plane
		glGenVertexArrays(1, &texturetestVertexArrayIDScreen);
		glBindVertexArray(texturetestVertexArrayIDScreen);
		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &texturetestVertexBufferIDScreen);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, texturetestVertexBufferIDScreen);
		vec3 vertices[6];
		vertices[0] = vec3(-1, -1, 0);
		vertices[1] = vec3(1, -1, 0);
		vertices[2] = vec3(1, 1, 0);
		vertices[3] = vec3(-1, -1, 0);
		vertices[4] = vec3(1, 1, 0);
		vertices[5] = vec3(-1, 1, 0);
		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(vec3), vertices, GL_STATIC_DRAW);
		//we need to set up the vertex array
		glEnableVertexAttribArray(0);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &texturetestVertexBufferTexScreen);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, texturetestVertexBufferTexScreen);
		vec2 texscreen[6];
		texscreen[0] = vec2(0, 0);
		texscreen[1] = vec2(1, 0);
		texscreen[2] = vec2(1, 1);
		texscreen[3] = vec2(0, 0);
		texscreen[4] = vec2(1, 1);
		texscreen[5] = vec2(0, 1);
		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(vec2), texscreen, GL_STATIC_DRAW);
		//we need to set up the vertex array
		glEnableVertexAttribArray(1);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glBindVertexArray(0);


		//generate the VAO
		glGenVertexArrays(1, &texturetestVertexArrayID);
		glBindVertexArray(texturetestVertexArrayID);

		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &texturetestVertexBufferID);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, texturetestVertexBufferID);

		GLfloat cube_vertices[] = {
			// front
			-1.0, -1.0,  1.0,//LD
			1.0, -1.0,  1.0,//RD
			1.0,  1.0,  1.0,//RU
			-1.0,  1.0,  1.0,//LU
		};
		//make it a bit smaller
		for (int i = 0; i < 12; i++)
			cube_vertices[i] *= 0.5;
		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_DYNAMIC_DRAW);

		//we need to set up the vertex array
		glEnableVertexAttribArray(0);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		//color
		GLfloat cube_norm[] = {
			// front colors
			0.0, 0.0, 1.0,
			0.0, 0.0, 1.0,
			0.0, 0.0, 1.0,
			0.0, 0.0, 1.0,

		};
		glGenBuffers(1, &texturetestVertexNormDBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, texturetestVertexNormDBox);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cube_norm), cube_norm, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		//color
		glm::vec2 cube_tex[] = {
			// front colors
			glm::vec2(0.0, 1.0),
			glm::vec2(1.0, 1.0),
			glm::vec2(1.0, 0.0),
			glm::vec2(0.0, 0.0),

		};
		glGenBuffers(1, &texturetestVertexTexBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, texturetestVertexTexBox);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cube_tex), cube_tex, GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glGenBuffers(1, &texturetestIndexBufferIDBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, texturetestIndexBufferIDBox);
		GLushort cube_elements[] = {

			// front
			0, 1, 2,
			2, 3, 0,
		};
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_elements), cube_elements, GL_STATIC_DRAW);


		//generate vertex buffer to hand off to OGL ###########################
		glGenBuffers(1, &texturetestInstanceBuffer);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, texturetestInstanceBuffer);
		glm::vec4* positions = new glm::vec4[500];
		for (int i = 0; i < 500; i++)
			positions[i] = glm::vec4(-250 + i, 0, -10, 0);
		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 500 * sizeof(glm::vec4), positions, GL_STATIC_DRAW);
		int position_loc = glGetAttribLocation(ptexturetest1->pid, "InstancePos");
		for (int i = 0; i < 500; i++)
		{
			// Set up the vertex attribute
			glVertexAttribPointer(position_loc + i,              // Location
				4, GL_FLOAT, GL_FALSE,       // vec4
				sizeof(vec4),                // Stride
				(void*)(sizeof(vec4) * i)); // Start offset
											 // Enable it
			glEnableVertexAttribArray(position_loc + i);
			// Make it instanced
			glVertexAttribDivisor(position_loc + i, 1);
		}

		glBindVertexArray(0);

		int width, height, channels;
		char filepath[1000];

		//texture 1
		string str = resourceDirectory + "/mario.png";
		strcpy(filepath, str.c_str());
		unsigned char* data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &texturetestTexture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texturetestTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		//texture 2
		str = resourceDirectory + "/sky.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &texturetestTexture2);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texturetestTexture2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		//texture 2
		str = resourceDirectory + "/tv.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &TVtex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, TVtex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		//[TWOTEXTURES]
		//set the 2 textures to the correct samplers in the fragment shader:
		GLuint Tex1Location = glGetUniformLocation(ptexturetest1->pid, "tex");//tex, tex2... sampler in the fragment shader
		GLuint Tex2Location = glGetUniformLocation(ptexturetest1->pid, "tex2");
		// Then bind the uniform samplers to texture units:
		glUseProgram(ptexturetest1->pid);
		glUniform1i(Tex1Location, 0);
		glUniform1i(Tex2Location, 1);

		Tex1Location = glGetUniformLocation(ptexturetestpostproc->pid, "tex");//tex, tex2... sampler in the fragment shader
		glUseProgram(ptexturetestpostproc->pid);
		glUniform1i(Tex1Location, 0);


		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		//RGBA8 2D texture, 24 bit depth texture, 256x256
		glGenTextures(1, &texturetestFBOtex);
		glBindTexture(GL_TEXTURE_2D, texturetestFBOtex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//NULL means reserve texture memory, but texels are undefined
		//**** Tell OpenGL to reserve level 0
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
		//You must reserve memory for other mipmaps levels as well either by making a series of calls to
		//glTexImage2D or use glGenerateMipmapEXT(GL_TEXTURE_2D).
		//Here, we'll use :
		glGenerateMipmap(GL_TEXTURE_2D);
		//make a frame buffer
		//-------------------------
		glGenFramebuffers(1, &texturetestfb);
		glBindFramebuffer(GL_FRAMEBUFFER, texturetestfb);
		//Attach 2D texture to this FBO
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texturetestFBOtex, 0);
		//-------------------------
		glGenRenderbuffers(1, &texturetestdepth_fb);
		glBindRenderbuffer(GL_RENDERBUFFER, texturetestdepth_fb);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
		//-------------------------
		//Attach depth buffer to FBO
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, texturetestdepth_fb);
		//-------------------------
		//Does the GPU support current FBO configuration?
		GLenum status;
		status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		switch (status)
		{
		case GL_FRAMEBUFFER_COMPLETE:
			cout << "status framebuffer: good";
			break;
		default:
			cout << "status framebuffer: bad!!!!!!!!!!!!!!!!!!!!!!!!!";
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

	}

	//**************************************************************************************************************************************************
	void initGeom(const std::string & resourceDirectory)
	{
		init_land();
		init_tunnel(resourceDirectory);
		texturetest_initGeom();
		//lasers:
		glGenVertexArrays(1, &VAOLasers);
		glBindVertexArray(VAOLasers);
		glGenBuffers(1, &VBLasersPos);
		glBindBuffer(GL_ARRAY_BUFFER, VBLasersPos);
		vec3 posi[MAXLASERS];
		for (int i = 0; i < MAXLASERS; i++)
			posi[i] = vec3(i * 3, 0, -3);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * MAXLASERS, posi, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			
		glGenBuffers(1, &VBLasersCol);
		glBindBuffer(GL_ARRAY_BUFFER, VBLasersCol);
		vec3 lascol[MAXLASERS];
		for (int i = 0; i < MAXLASERS; i++)
			lascol[i] = vec3(0);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * MAXLASERS, lascol, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glGenBuffers(1, &VBLasersDir);
		glBindBuffer(GL_ARRAY_BUFFER, VBLasersDir);
		vec3 lasdir[MAXLASERS];

		for (int i = 0; i < MAXLASERS; i++)
			lasdir[i] = vec3(0,1,0);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * MAXLASERS, lasdir, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glBindVertexArray(0);
		//------------------------------------------------


		for (int i = 0; i < 10; i++)
		{
			amplitude_on_frequency_10steps[i] = 0;
		}
		//init rectangle mesh (2 triangles) for the post processing
		glGenVertexArrays(1, &VertexArrayIDBox);
		glBindVertexArray(VertexArrayIDBox);

		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferIDBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferIDBox);

		GLfloat* rectangle_vertices = new GLfloat[18];
		// front
		int verccount = 0;

		rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = -1, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;


		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), rectangle_vertices, GL_STATIC_DRAW);
		//we need to set up the vertex array
		glEnableVertexAttribArray(0);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);


		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferTex);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferTex);

		float t = 1. / 100.;
		GLfloat* rectangle_texture_coords = new GLfloat[12];
		int texccount = 0;
		rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 0;
		rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 0;
		rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 1;
		rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 0;
		rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 1;
		rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 1;

		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), rectangle_texture_coords, GL_STATIC_DRAW);
		//we need to set up the vertex array
		glEnableVertexAttribArray(2);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		face = make_shared<Shape>();
		face->loadMesh(resourceDirectory + "/face2.obj");
		face->resize();
		face->init();

		eyes = make_shared<Shape>();
		eyes->loadMesh(resourceDirectory + "/eyes.obj");
		eyes->resize();
		eyes->init();

		armor = make_shared<Shape>();
		armor->loadMesh(resourceDirectory + "/armor2.obj");
		armor->resize();
		armor->init();

		sphere = make_shared<Shape>();
		//sphere->loadMesh(resourceDirectory + "/sky.obj");
		sphere->loadMesh(resourceDirectory + "/sphere.obj");
		sphere->resize();
		sphere->init();
		// Initialize mesh.
		shape = make_shared<Shape>();
		shape->loadMesh(resourceDirectory + "/buildings3.obj");
		shape->resize_subobj();
		float abst = BULDINGDIST;
		int instancesize = shape->return_subsize();
		vector<float> * instancepos = new vector<float>[instancesize];

		for (int xx = -CITYSLOTS; xx < CITYSLOTS; xx++)
			for (int zz = -CITYSLOTS; zz < CITYSLOTS; zz++)
			{
				float height = (float)rand() / 32768.;
				height = pow(height, 20);
				height *= 5;
				if (height < 0.5) height = 0.5;
				int house = rand() % instancesize;					
				if (xx == 0) height = min(2.0, height);
				if (xx == 1) height = min(2.0, height);
				if (xx == -1) height = min(2.0, height);
				instancepos[house].push_back(xx * abst);
				instancepos[house].push_back(height);
				instancepos[house].push_back(zz * abst);
				instancepos[house].push_back(rand() % 6);
				cityfield[xx + CITYSLOTS][zz + CITYSLOTS].height = height;
			}
		init_laser();
		for (int ii = 0; ii < shape->return_subsize(); ii++)
		{
			shape->init(ii, true, 3, &instancepos[ii], sizeof(vec4));
		}
		delete[] instancepos;
			
		//spheres

		// Initialize mesh.
		sphere_land = make_shared<Shape>();
		sphere_land->loadMesh(resourceDirectory + "/sphere.obj");
		sphere_land->resize();
		vector<float> instancepossphere;
		//for (int zz = 0; zz < SPHERECOUNT; zz++)
		//	for (int ii =0; ii < SPHERECOUNT; ii++)
		//	{				
		//		instancepossphere.push_back(ii*2 - 50*2);
		//		instancepossphere.push_back(-zz*2);
		//		instancepossphere.push_back(1); //rotation
		//		instancepossphere.push_back(0.2);//size
		//	}			


		for (int ii = 0; ii < SPHERECOUNT; ii++)
		{
			vec3 pos = vec3(frand() * SPHEREDISTANCEZ  - SPHEREDISTANCEZ / 2,
							frand() * 5 - 2.5,
							-frand() * SPHEREDISTANCEZ*2
							);
			pos.x = floor(pos.x);
			pos.z = floor(pos.z);
			instancepossphere.push_back(pos.x);
			instancepossphere.push_back(pos.y);
			instancepossphere.push_back(pos.z); //rotation
			instancepossphere.push_back(frand() * 1.5 + 0.2);//size
		}
		for (int ii = 0; ii < 50; ii++)
		{
			vec3 pos = vec3(frand() * 5 - 2.5,
							frand() * 6,
							-frand() * SPHEREDISTANCEZ * 2
							);
			pos.x = floor(pos.x);
			pos.z = floor(pos.z);
			instancepossphere.push_back(pos.x);
			instancepossphere.push_back(pos.y);
			instancepossphere.push_back(pos.z); //rotation
			instancepossphere.push_back(0.08);//size
		}

		sphere_land->init(0, true, 3, &instancepossphere, sizeof(vec4));
				
		// Initialize rectangles.
		instancepossphere.clear();
		rects = make_shared<Shape>();
		rects->loadMesh(resourceDirectory + "/rect.obj");
		rects->resize();
		for (int zz = 0; zz < BILLCOUNT; zz++)
		{
			for (int ii = 0; ii < BILLCOUNT; ii++)
			{
				instancepossphere.push_back(ii * 2 - 50 * 2);
				instancepossphere.push_back(0);
				instancepossphere.push_back(-zz * 2); //rotation
				instancepossphere.push_back(0.3);//size
			}
		}

		rects->init(0, true, 3, &instancepossphere, sizeof(vec4));
			
		//	--------------------------------------------- TEXTURES -----------------------------------
		int width, height, channels;
		char filepath[1000];

		//texture earth diffuse
		string str = resourceDirectory + "/sky.jpg";
		str = resourceDirectory + "/sunsets4.jpg";
		strcpy(filepath, str.c_str());
		unsigned char* data = stbi_load(filepath, &width, &height, &channels, 4);
		TextureSky = generate_texture2D(GL_RGBA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data, GL_MIRRORED_REPEAT, GL_LINEAR, GL_LINEAR);
		//texture earth diffuse
		str = resourceDirectory + "/landBG.jpg";
		strcpy(filepath, str.c_str());
			data = stbi_load(filepath, &width, &height, &channels, 4);
		TextureLandBG = generate_texture2D(GL_RGBA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data, GL_MIRRORED_REPEAT, GL_LINEAR, GL_LINEAR);
		//texture earth diffuse
		str = resourceDirectory + "/landBGgod.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		TextureLandBGgod = generate_texture2D(GL_RGBA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data, GL_MIRRORED_REPEAT, GL_LINEAR, GL_LINEAR);
		//texture earth diffuse
		str = resourceDirectory + "/skyscraperBG.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		TextureEarth = generate_texture2D(GL_RGBA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data, GL_REPEAT, GL_LINEAR, GL_LINEAR);
		//texture earth diffuse
		str = resourceDirectory + "/roundlaser.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		TexTunnelLaser = generate_texture2D(GL_RGBA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data, GL_REPEAT, GL_LINEAR, GL_LINEAR);			
		//texture earth diffuse
		str = resourceDirectory + "/laser2.tga";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		TexLaser = generate_texture2D(GL_RGBA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data, GL_REPEAT, GL_LINEAR, GL_LINEAR);
		//texture earth diffuse
		str = resourceDirectory + "/lensdirt5.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		TexLensdirt = generate_texture2D(GL_RGBA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data, GL_REPEAT, GL_LINEAR, GL_LINEAR);
		//texture earth diffuse
		str = resourceDirectory + "/noise.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		TexNoise = generate_texture2D(GL_RGBA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data, GL_REPEAT, GL_LINEAR, GL_LINEAR);
		//texture earth diffuse
		str = resourceDirectory + "/sunsetsgod.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		TexGod = generate_texture2D(GL_RGBA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data, GL_MIRRORED_REPEAT, GL_LINEAR, GL_LINEAR);
		//texture earth diffuse
		str = resourceDirectory + "/masklaser.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		TexLaserMask = generate_texture2D(GL_RGBA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
		//texture earth diffuse
		str = resourceDirectory + "/texturesuit.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		TexSuit = generate_texture2D(GL_RGBA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data, GL_REPEAT, GL_LINEAR, GL_LINEAR);
		//texture earth diffuse
		str = resourceDirectory + "/mask.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		TexSuitMask = generate_texture2D(GL_RGBA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data, GL_REPEAT, GL_LINEAR, GL_LINEAR);

		//audio texture
		BYTE datamanual[TEXELSIZE * TEXELSIZE * 4];
		for (int ii = 0; ii < TEXELSIZE * TEXELSIZE * 4; ii++)
			datamanual[ii] = 0;
		TexAudio = generate_texture2D(GL_RGBA8, TEXELSIZE, TEXELSIZE, GL_RGBA, GL_UNSIGNED_BYTE, datamanual, GL_REPEAT, GL_LINEAR, GL_LINEAR);
			

		//texture front
		for (int ii = 0; ii < 6; ii++)
		{
			char text[1000];
			sprintf(text, "%s/skyscraperF%d.jpg", resourceDirectory.c_str(), ii + 1);
			str = text;
			strcpy(filepath, str.c_str());
			data = stbi_load(filepath, &width, &height, &channels, 4);
			TextureF[ii] = generate_texture2D(GL_RGBA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data, GL_REPEAT, GL_LINEAR, GL_LINEAR);
		}
		//[TWOTEXTURES]
		//set the 2 textures to the correct samplers in the fragment shader:
		GLuint Tex1Location = glGetUniformLocation(prog->pid, "tex");//tex, tex2... sampler in the fragment shader
		GLuint Tex2Location = glGetUniformLocation(prog->pid, "tex2");
		// Then bind the uniform samplers to texture units:
		glUseProgram(prog->pid);
		glUniform1i(Tex1Location, 0);
		glUniform1i(Tex2Location, 1);

		generate_flexible_framebuffers();

		//create frame buffer for cube map
		glGenFramebuffers(1, &Cube_framebuffer);
		glActiveTexture(GL_TEXTURE0);
		glBindFramebuffer(GL_FRAMEBUFFER, Cube_framebuffer);
		//create depth buffer
		glGenRenderbuffers(1, &Cube_depthbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, Cube_depthbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, CUBE_TEXTURE_SIZE, CUBE_TEXTURE_SIZE);
		//Attach depth buffer to FBO
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, Cube_depthbuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		CreateCubeTexture();
		//-------------------------

		//Does the GPU support current FBO configuration?
		glUseProgram(prog->pid);
		int TexLoc = glGetUniformLocation(prog->pid, "tex");		glUniform1i(TexLoc, 0);
		TexLoc = glGetUniformLocation(prog->pid, "tex1");			glUniform1i(TexLoc, 1);
		TexLoc = glGetUniformLocation(prog->pid, "tex2");			glUniform1i(TexLoc, 2);
		TexLoc = glGetUniformLocation(prog->pid, "tex3");			glUniform1i(TexLoc, 3);
		TexLoc = glGetUniformLocation(prog->pid, "tex4");			glUniform1i(TexLoc, 4);
		TexLoc = glGetUniformLocation(prog->pid, "tex5");			glUniform1i(TexLoc, 5);
		TexLoc = glGetUniformLocation(prog->pid, "tex6");			glUniform1i(TexLoc, 6);
		TexLoc = glGetUniformLocation(prog->pid, "skytex");			glUniform1i(TexLoc, 7);
		TexLoc = glGetUniformLocation(prog->pid, "skycube");
		glUniform1i(TexLoc, 8);

		glUseProgram(progs->pid);
		TexLoc = glGetUniformLocation(progs->pid, "tex");			glUniform1i(TexLoc, 0);
		TexLoc = glGetUniformLocation(progs->pid, "tex1");			glUniform1i(TexLoc, 1);
		TexLoc = glGetUniformLocation(progs->pid, "skycube");		glUniform1i(TexLoc, 8);

		glUseProgram(progsky->pid);
		TexLoc = glGetUniformLocation(progsky->pid, "tex");			glUniform1i(TexLoc, 0);
		TexLoc = glGetUniformLocation(progsky->pid, "texgod");		glUniform1i(TexLoc, 1);

		glUseProgram(prog_laser->pid);
		TexLoc = glGetUniformLocation(prog_laser->pid, "tex");		glUniform1i(TexLoc, 0);
		TexLoc = glGetUniformLocation(prog_laser->pid, "mask");		glUniform1i(TexLoc, 1);

		glUseProgram(prog_lasereyes->pid);
		TexLoc = glGetUniformLocation(prog_lasereyes->pid, "tex");		glUniform1i(TexLoc, 0);
		TexLoc = glGetUniformLocation(prog_lasereyes->pid, "mask");		glUniform1i(TexLoc, 1);

		glUseProgram(prog_blur->pid);
		TexLoc = glGetUniformLocation(prog_blur->pid, "colortex");		glUniform1i(TexLoc, 0);
		TexLoc = glGetUniformLocation(prog_blur->pid, "maskmap");		glUniform1i(TexLoc, 1);

		glUseProgram(prog_bloom->pid);
		TexLoc = glGetUniformLocation(prog_bloom->pid, "colortex");		glUniform1i(TexLoc, 0);
		TexLoc = glGetUniformLocation(prog_bloom->pid, "maskmap");		glUniform1i(TexLoc, 1);
			
		glUseProgram(prog2->pid);
		TexLoc = glGetUniformLocation(prog2->pid, "tex");			glUniform1i(TexLoc, 0);
		TexLoc = glGetUniformLocation(prog2->pid, "viewpos");		glUniform1i(TexLoc, 1);
		TexLoc = glGetUniformLocation(prog2->pid, "worldpos");		glUniform1i(TexLoc, 2);
		TexLoc = glGetUniformLocation(prog2->pid, "worldnormal");	glUniform1i(TexLoc, 3);
		TexLoc = glGetUniformLocation(prog2->pid, "noise");			glUniform1i(TexLoc, 4);
		TexLoc = glGetUniformLocation(prog2->pid, "skygod");		glUniform1i(TexLoc, 5);
		TexLoc = glGetUniformLocation(prog2->pid, "bloommap");		glUniform1i(TexLoc, 6);
		TexLoc = glGetUniformLocation(prog2->pid, "lensdirt");		glUniform1i(TexLoc, 7);
		TexLoc = glGetUniformLocation(prog2->pid, "skycube");		glUniform1i(TexLoc, 8);

		GLenum status;
		status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		switch (status)
		{
			case GL_FRAMEBUFFER_COMPLETE:
			cout << "status framebuffer: good";
			break;
			default:
			cout << "status framebuffer: bad!!!!!!!!!!!!!!!!!!!!!!!!!";
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);


	}

	//*************************************
	double get_last_elapsed_time()
	{
		static double lasttime = glfwGetTime();
		double actualtime = glfwGetTime();
		double difference = actualtime - lasttime;
		lasttime = actualtime;
		return difference;
	}

	//*************************************
	void render_merge_to_postproc()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, FBO_blur);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOcolor_no_ssaa[0], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, FBOcolor_blurmap, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, FBOcolor_bloommap, 0);
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 , GL_COLOR_ATTACHMENT2};
		glDrawBuffers(3, buffers);

		//glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Get current frame buffer size.
		int width, height;
		get_resolution(&width, &height);
	
		glViewport(0, 0, width, height);

		glm::mat4 M, V, S, T;

		prog2->bind();

		V = glm::mat4(1);

		mat4 Vcam = mycam.V;
		mat4 Pcam = glm::perspective((float)(3.14159 / 3.), (float)((float)width / (float)height), 0.1f, 100.0f); //so much type casting... GLM metods are quite funny ones
		glUniformMatrix4fv(prog2->getUniform("Pcam"), 1, GL_FALSE, &Pcam[0][0]);
		glUniformMatrix4fv(prog2->getUniform("Vcam"), 1, GL_FALSE, &Vcam[0][0]);

		glUniform1f(prog2->getUniform("speed"), mycam.get_speed());

		// Clear framebuffer.

		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);		glBindTexture(GL_TEXTURE_2D, FBOcolor);
		glActiveTexture(GL_TEXTURE1);		glBindTexture(GL_TEXTURE_2D, FBOviewpos);
		glActiveTexture(GL_TEXTURE2);		glBindTexture(GL_TEXTURE_2D, FBOworldpos);
		glActiveTexture(GL_TEXTURE3);		glBindTexture(GL_TEXTURE_2D, FBOworldnormal);
		glActiveTexture(GL_TEXTURE4);		glBindTexture(GL_TEXTURE_2D, TexNoise);
		glActiveTexture(GL_TEXTURE5);		glBindTexture(GL_TEXTURE_2D, FBOgodrays);
		glActiveTexture(GL_TEXTURE6);		glBindTexture(GL_TEXTURE_2D, FBObloommask);
		glActiveTexture(GL_TEXTURE7);		glBindTexture(GL_TEXTURE_2D, TexLensdirt);
		glActiveTexture(GL_TEXTURE8);		glBindTexture(GL_TEXTURE_CUBE_MAP, g_cubeTexture);

		V = glm::mat4(1);
		
		glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(prog2->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(prog2->getUniform("M"), 1, GL_FALSE, &V[0][0]);
		vec2 winsize = vec2(width, height);
		glUniform2fv(prog2->getUniform("windows_size"), 1, &winsize.x);
		glBindVertexArray(VertexArrayIDBox);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		prog2->unbind();
		glBindTexture(GL_TEXTURE_2D, FBOcolor_no_ssaa[0]);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOcolor_blurmap);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOcolor_bloommap);
		glGenerateMipmap(GL_TEXTURE_2D);

		
	}

	//*****************************************************************************************************************************************************************************
	void render_postproc(double frametime, int step,bool bluron, std::shared_ptr<Program> & postprocprog,bool secondstep_loops, GLuint textarget,GLuint texsource_1,GLuint texsource_2) // aka render to framebuffer
	{			
		if (step == 0)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, FBO_blur);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textarget, 0);
			GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
			glDrawBuffers(1, buffers);
		}
		else if (secondstep_loops)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, FBO_blur);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textarget, 0);
			GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
			glDrawBuffers(1, buffers);
		}
		else 
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
			
		glClearColor(1.0, 0.0, 0.0, 1.0);
		int width, height;
		get_resolution(&width, &height); 

		int realwidth, realheight;
		glfwGetFramebufferSize(windowManager->getHandle(), &realwidth, &realheight);
		if (step == 1)glViewport(0, 0, realwidth, realheight);
			
		glm::mat4 V = mat4(1), P = mat4(1);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		postprocprog->bind();

		V = glm::mat4(1);

		mat4 Vcam = mycam.V;
		mat4 Pcam = glm::perspective((float)(3.14159 / 3.), (float)((float)width / (float)height), 0.1f, 100.0f); //so much type casting... GLM metods are quite funny ones
		glUniformMatrix4fv(postprocprog->getUniform("Pcam"), 1, GL_FALSE, &Pcam[0][0]);
		glUniformMatrix4fv(postprocprog->getUniform("Vcam"), 1, GL_FALSE, &Vcam[0][0]);
		glUniform1f(postprocprog->getUniform("speed"), mycam.get_speed());
		glUniform1f(postprocprog->getUniform("stepno"), (float)step);
		glUniform1f(postprocprog->getUniform("bluractive"), (float)bluron);
		vec2 winsize = vec2(width, height);
		glUniform2fv(postprocprog->getUniform("windows_size"),1, &winsize.x);
		glUniformMatrix4fv(postprocprog->getUniform("P"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(postprocprog->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(postprocprog->getUniform("M"), 1, GL_FALSE, &V[0][0]);

		glActiveTexture(GL_TEXTURE0);		glBindTexture(GL_TEXTURE_2D, texsource_1);
		glActiveTexture(GL_TEXTURE1);		glBindTexture(GL_TEXTURE_2D, texsource_2);				

		glBindVertexArray(VertexArrayIDBox);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		/*if (step == 0)
			{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glBindTexture(GL_TEXTURE_2D, FBOcolor_no_ssaa[1]);
			glGenerateMipmap(GL_TEXTURE_2D);
			}		*/
	}

	void update_audio_influence()
	{
		float peak_average = 0;
		float sample_slots = 13;
		for (int i = 0; i < sample_slots; i++)
		{
			peak_average += amplitude_on_frequency[i];
		}
		peak_average = peak_average / sample_slots;
		//cout << "peak average: " << peak_average << endl;
		global_music_influence = peak_average;
		//cout << global_music_influence << endl;
		//cout << "global_music: " << global_music_influence << endl;
	}

	//*************************************
	void aquire_fft_scaling_arrays()
	{
		static float weights[10];
		static bool first = true;
		if (first)
		{
			for (int ii = 0; ii < 10; ii++)
				weights[ii] = 1. + pow((float)(ii / 10.0), 2)*15.;
			first = false;
			//weights[0] = 0.02;
			/*weights[1] = 0.5;
			weights[2] = 0.8;*/
		}
		//get FFT array
		static int length = 0;
		if (fft(amplitude_on_frequency, length))
		{
			//Update the global audio influence
			update_audio_influence();
			//put the height of the frequencies 20Hz to 20000Hz into the height of the line-vertices
			vec3 vertices[FFT_MAXSIZE];

			float oldval[10];
			//calculate the average amplitudes for the 10 spheres
			for (int i = 0; i < 10; i++)
			{
				oldval[i] = amplitude_on_frequency_10steps[i];

				amplitude_on_frequency_10steps[i] = 0;
			}
			int mean_range = length / 10;
			int bar = 0;
			int count = 0;
			mean_range = 1;
			for (int i = 0; ; i++, count++)
			{
				if (mean_range == count)
				{
					count = -1;
					amplitude_on_frequency_10steps[bar] /= (float)mean_range;
					bar++;
					mean_range++;
					if (bar == 10)
					{
						break;
					}
				}
				if (i < length && i < FFT_MAXSIZE)
					amplitude_on_frequency_10steps[bar] += amplitude_on_frequency[i];
			}
			for (int i = 0; i < 10; i++)
			{
				amplitude_on_frequency_10steps[i] = delayfilter(oldval[i], amplitude_on_frequency_10steps[i], 1.02);
			}
		}

		frequency_to_texture(TexAudio, TEXELSIZE, TEXELSIZE, amplitude_on_frequency_10steps);
			
		//frequency to roundlaser
		float frequlas[FFT_REALSIZE *2];
		glBindBuffer(GL_ARRAY_BUFFER, VB_roundlaser_frequ); 
		int frequpart = FFT_MAXSIZE / 10;
		for (int i = 0; i < FFT_REALSIZE; i++)
		{
			if (i >= FFT_REALSIZE / 2)
			{
				frequlas[i * 2 + 0] = amplitude_on_frequency[FFT_REALSIZE - i] * weights[(FFT_REALSIZE - i) / frequpart];
				frequlas[i * 2 + 1] = amplitude_on_frequency[FFT_REALSIZE - i] * weights[(FFT_REALSIZE - i) / frequpart];
			}
			else
			{
				frequlas[i * 2 + 0] = amplitude_on_frequency[i] * weights[i / frequpart];
				frequlas[i * 2 + 1] = amplitude_on_frequency[i] * weights[i / frequpart];
			}
		}
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * FFT_REALSIZE *2, frequlas);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
	//************************************************************************************************
	void update_wobble_pilot(double frametime)
	{
		if (rendermode != MODE_CITYFWD) return;
		static float Ax = 1, Ay = 1, Az = 1;
		static vec3 Periods = vec3(1);
		static vec3 totalperiod = vec3(0);
		totalperiod += Periods * (float)frametime * (float)3.2;

		if (totalperiod.x <= 0.00001 || totalperiod.x >= 3.1415926)
		{
			Periods.x = 0.5 + frand();
			float newAx = (frand() + 0.6) * 0.00027;
			if (Ax > 0)
			{
				newAx *= -1;
			}
			Ax = newAx;
			totalperiod.x = 0;
		}

		if (totalperiod.y <= 0.00001 || totalperiod.y >= 3.1415926)
		{
			Periods.y = 0.5 + frand();
			float newAy = (frand() + 0.6) * 0.00027;
			if (Ay > 0)
			{
				newAy *= -1;
			}
			Ay = newAy;
			totalperiod.y = 0;
		}

		if (totalperiod.z <= 0.00001 || totalperiod.z >= 3.1415926)
		{
			Periods.z = 0.5 + frand();
			float newAz = (frand() + 0.2) * 0.03;
			if (Ax > 0)
			{
				newAz *= -1;
			}
			Az = newAz;
			totalperiod.z = 0;
		}

		vec3 wobble;
		wobble.x = sin(totalperiod.x) * Ax;
		wobble.y = sin(totalperiod.y) * Ay;
		wobble.z = sin(totalperiod.z) * Az;
		mat4 VT = translate(mat4(1), vec3(wobble.x, wobble.y, 0));
		mat4 VRz = rotate(mat4(1), wobble.z, vec3(0, 0, 1));

		M_wobble = VT * VRz;
	}

	//************************************************************************************************	
	void render_citypilot(int width, int height, mat4 P, mat4 V, bool pilotrender, double frametime)
	{

		float mul_scale = CITYMULSCALE;

		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Get current frame buffer size.

		glViewport(0, 0, width, height);

		glm::mat4 M, S, T;

		float pih = -3.1415926 / 2.0;
		glm::mat4 Rx = glm::rotate(glm::mat4(1.f), pih, glm::vec3(1, 0, 0));

		//	******		skysphere		******

		glFrontFace(GL_CW);
		glDisable(GL_DEPTH_TEST);

		glm::mat4 RxSun = glm::rotate(glm::mat4(1.f), (float)(3.1415926 * 0.5), glm::vec3(1, 0, 0));
		glm::mat4 RySun = glm::rotate(glm::mat4(1.f), (float)(-3.1415926 * 0.5 + 0.8), glm::vec3(0, 1, 0));
		T = glm::translate(glm::mat4(1.f), glm::vec3(mycam.pos));
		S = glm::scale(glm::mat4(1.f), glm::vec3(100.1, 100.1, 100.1));

		progsky->bind();

		glActiveTexture(GL_TEXTURE0);			glBindTexture(GL_TEXTURE_2D, TextureSky);
		glActiveTexture(GL_TEXTURE1);			glBindTexture(GL_TEXTURE_2D, TexGod);

		glUniformMatrix4fv(progsky->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(progsky->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		float ww = (2. * 3.1415926) / 8.;

		mat4 MM = T * RySun * RxSun * S;
		glUniformMatrix4fv(progsky->getUniform("M"), 1, GL_FALSE, &MM[0][0]);
		sphere->draw(prog, true);	//draw sky

		progsky->unbind();

		//****************boden********************
		progg->bind();
		glUniformMatrix4fv(progg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(progg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		for (int ii = 0; ii < 2; ii++)
		{
			M = translate(mat4(1), vec3(0, -40, -ii * CITYMULSCALE * CITYSIZE)) * glm::rotate(glm::mat4(1), (float)(3.1415926 * 0.5), glm::vec3(1, 0, 0)) * glm::scale(glm::mat4(1), glm::vec3(CITYSIZE * CITYMULSCALE * 0.5, CITYSIZE * CITYMULSCALE * 0.5, 1));
			//M = translate(mat4(1), vec3(0, 0, -4+ii*8))* glm::scale(glm::mat4(1), glm::vec3(20,20,20));
			glUniformMatrix4fv(progg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			glBindVertexArray(VertexArrayIDBox);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		progg->unbind();
		//**************** ENDE boden*********************
		glFrontFace(GL_CCW);
		glEnable(GL_DEPTH_TEST);

		//bind shader and copy matrices
		prog->bind();

		glUniform1f(prog->getUniform("reflection_on"), 0.0);

		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniform3fv(prog->getUniform("campos"), 1, &mycam.pos.x);

		//	******		buildings		******
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureEarth);
		glActiveTexture(GL_TEXTURE1);		glBindTexture(GL_TEXTURE_2D, TextureF[0]);
		glActiveTexture(GL_TEXTURE2);		glBindTexture(GL_TEXTURE_2D, TextureF[1]);
		glActiveTexture(GL_TEXTURE3);		glBindTexture(GL_TEXTURE_2D, TextureF[2]);
		glActiveTexture(GL_TEXTURE4);		glBindTexture(GL_TEXTURE_2D, TextureF[3]);
		glActiveTexture(GL_TEXTURE5);		glBindTexture(GL_TEXTURE_2D, TextureF[4]);
		glActiveTexture(GL_TEXTURE6);		glBindTexture(GL_TEXTURE_2D, TextureF[5]);
		glActiveTexture(GL_TEXTURE7);		glBindTexture(GL_TEXTURE_2D, TextureSky);
		glActiveTexture(GL_TEXTURE8);		glBindTexture(GL_TEXTURE_CUBE_MAP, g_cubeTexture);
		glUniform1fv(prog->getUniform("freq_ampl"), 10, amplitude_on_frequency_10steps);
		//cout << amplitude_on_frequency_10steps[3] << endl;
	
		float offset_z = 0;
		int parts = (int)mycam.pos.z / (CITYSIZE * mul_scale);
		offset_z = parts * (CITYSIZE * mul_scale);
		if (parts != 0)
		{
			mycam.pos.z -= offset_z;
		}
			
		for (int ii = 0; ii < 2; ii++)
		{
			M = glm::translate(glm::mat4(1.f), glm::vec3(0, -2.9 * mul_scale, offset_z - CITYSIZE * mul_scale * ii)) * scale(mat4(1), vec3(mul_scale, mul_scale, mul_scale));
			glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			for (int ii = 0; ii < shape->return_subsize(); ii++)
				shape->draw(ii, prog, true);	//draw earth
		}
		prog->unbind();
			
		//	******		//buildings		******

			//************************** LASER *******************************
		if (initialized_VB_laser_count > 0)
			{
			prog_laser->bind();
			glUniformMatrix4fv(prog_laser->getUniform("P"), 1, GL_FALSE, &P[0][0]);
			glUniformMatrix4fv(prog_laser->getUniform("V"), 1, GL_FALSE, &V[0][0]);
			glActiveTexture(GL_TEXTURE0);			glBindTexture(GL_TEXTURE_2D, TexLaser);
			glActiveTexture(GL_TEXTURE1);			glBindTexture(GL_TEXTURE_2D, TexLaserMask);
			glBindVertexArray(VAOLasers);

			for (int ii = 1; ii >= 0; ii--)
				{
				M = translate(mat4(1), vec3(0, 0, -4 - CITYSIZE * mul_scale * ii));
				glUniformMatrix4fv(prog_laser->getUniform("M"), 1, GL_FALSE, &M[0][0]);
				glDrawArrays(GL_POINTS, 0, initialized_VB_laser_count);
				}
			prog_laser->unbind();
			}
		//*********************************************
		// ******   pilot ******************************

		if (pilotrender && rendermode == MODE_CITYFWD)
		{
			glDisable(GL_BLEND);
			progs->bind();	

			glActiveTexture(GL_TEXTURE0);		glBindTexture(GL_TEXTURE_2D, TexSuit);
			glActiveTexture(GL_TEXTURE1);		glBindTexture(GL_TEXTURE_2D, TexSuitMask);
			glActiveTexture(GL_TEXTURE8);		glBindTexture(GL_TEXTURE_CUBE_MAP, g_cubeTexture);

			glUniform1f(progs->getUniform("reflection_on"), (float)pilotrender);
			P = glm::perspective((float)(3.14159 / 3.), (float)((float)width / (float)height), 0.001f, 1.0f); //so much type casting... GLM metods are quite funny ones
			glUniformMatrix4fv(progs->getUniform("P"), 1, GL_FALSE, &P[0][0]);
			glUniformMatrix4fv(progs->getUniform("V"), 1, GL_FALSE, &V[0][0]);
			glUniform3fv(progs->getUniform("campos"), 1, &mycam.pos.x);
			mat4 Sa = scale(mat4(1), vec3(0.015, 0.015, 0.015));

			static float zangle = 0;
			zangle = delay(zangle, z_schwenk, 0.02);
			//cout << zangle << endl;
			zangle = mycam.rot_diff().y;
			mat4 Rza = rotate(mat4(1), (float)(zangle), vec3(0, 0, 1));
			mat4 Rxa = rotate(mat4(1), (float)(-3.1415926 * 0.5 + 0.25 + (0.55 * !mycam.toggleview)), vec3(1, 0, 0));
			mat4 Rya = rotate(mat4(1), (float)(3.1415926 * 0.5), vec3(0, 1, 0));
			mat4 Ta = glm::translate(glm::mat4(1.f), glm::vec3(0, -0.014 * !mycam.toggleview + (-0.002 * mycam.toggleview), -0.018 + 0.002 * !mycam.toggleview));
			mat4 Tcam = glm::translate(glm::mat4(1.f), mycam.pos);

			M = Tcam * mycam.R * Ta * Rza * Rxa * Rya * Sa;
			glUniformMatrix4fv(progs->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			armor->draw(progs, true);
			progs->unbind();
			glEnable(GL_BLEND);	
		}
	}

	//************************************************************************************************	
	void render_land(int width, int height, mat4 P, mat4 V, bool pilotrender, double frametime)
	{
		float camstartoffset = 3.0;
		camstartoffset = 0.0;
		float mul_scale = CITYMULSCALE;

		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Get current frame buffer size.
		glViewport(0, 0, width, height);

		glm::mat4 M, S, T;

		float pih = -3.1415926 / 2.0;
		glm::mat4 Rx = glm::rotate(glm::mat4(1.f), pih, glm::vec3(1, 0, 0));

		//	******		skysphere		******
		glFrontFace(GL_CW);
		glDisable(GL_DEPTH_TEST);

		glm::mat4 RxSun = glm::rotate(glm::mat4(1.f), (float)(3.1415926 * 0.5), glm::vec3(1, 0, 0));
		glm::mat4 RySun = glm::rotate(glm::mat4(1.f), (float)(-3.1415926 * 0.5 + 0.8), glm::vec3(0, 1, 0));
		T = glm::translate(glm::mat4(1.f), glm::vec3(mycam.pos));
		S = glm::scale(glm::mat4(1.f), glm::vec3(100.1, 100.1, 100.1));

		progsky->bind();

		glActiveTexture(GL_TEXTURE0);			glBindTexture(GL_TEXTURE_2D, TextureLandBG);
		glActiveTexture(GL_TEXTURE1);			glBindTexture(GL_TEXTURE_2D, TextureLandBGgod);

		glUniformMatrix4fv(progsky->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(progsky->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		float ww = (2. * 3.1415926) / 8.;

		mat4 MM = T * RySun * RxSun * S;
		glUniformMatrix4fv(progsky->getUniform("M"), 1, GL_FALSE, &MM[0][0]);
		sphere->draw(prog, true);	//draw sky

		progsky->unbind();

		// ****************** face  ******************		
		glDisable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		prog_face->bind();
		glUniformMatrix4fv(prog_face->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(prog_face->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		static int face_rot = 0;
		static float yrot = 0.0;
		if (face_rot == 0)
			yrot -= frametime*0.1;
		else 
			yrot += frametime * 0.1;
		if (yrot < -0.3) face_rot = 1;
		if (yrot > 0.3) face_rot = 0;

		mycam.rot.z = yrot*0.5;
		//	
		M_facetrinity = translate(mat4(1), vec3(0, 60, -300+ mycam.pos.z )) * glm::scale(glm::mat4(1), glm::vec3(130, 130, 100)) * glm::rotate(glm::mat4(1), (float)(-0.3), glm::vec3(1, 0, 0))* glm::rotate(glm::mat4(1), yrot, glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1), glm::vec3(1,1,-1));
		glUniformMatrix4fv(prog_face->getUniform("M"), 1, GL_FALSE, &M_facetrinity[0][0]);
		vec3 extcolor = vec3(1,0,1)*0.3f;
		glUniform3fv(prog_face->getUniform("extcolor"), 1, &extcolor.x);
		face->draw(0, prog_face, true);	//draw sphere
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		extcolor = vec3(0, 1, 1);
		glUniform3fv(prog_face->getUniform("extcolor"), 1, &extcolor.x);
		eyes->draw(0, prog_face, true);	//draw sphere
		prog_face->unbind();
				
		//****************boden LINES ********************
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_CULL_FACE);

		glFrontFace(GL_CW);
		glEnable(GL_DEPTH_TEST);
			
		float zoffset = (float)((int)(mycam.pos.z/2.0 ));
		zoffset *= 2;
		vec3 offvec = vec3(0, 0, zoffset);
		glBindVertexArray(VAOland);

		progland2->bind();
		glActiveTexture(GL_TEXTURE0);			glBindTexture(GL_TEXTURE_2D, TexAudio);
		glUniformMatrix4fv(progland->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(progland->getUniform("V"), 1, GL_FALSE, &V[0][0]);				
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBlandfull);
		glUniform3fv(progland->getUniform("offset"), 1, &offvec.x);
		vec3 screen_offset = vec3(0, 0, 0);
		glUniform3fv(progland->getUniform("screen_offset"), 1, &screen_offset.x);
		M = mat4(1);// translate(mat4(1), vec3(0, 0, -ii * CITYMULSCALE * CITYSIZE))* glm::rotate(glm::mat4(1), (float)(3.1415926 * 0.5), glm::vec3(1, 0, 0))* glm::scale(glm::mat4(1), glm::vec3(CITYSIZE * CITYMULSCALE * 0.5, CITYSIZE * CITYMULSCALE * 0.5, 1));
		M = translate(mat4(1), vec3(0.5, -4, zoffset + camstartoffset));
		glUniformMatrix4fv(progland->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glDrawElements(GL_TRIANGLES, IBlandfullsize, GL_UNSIGNED_INT, (void*)0);
		progland2->unbind();

		progland->bind();
		glActiveTexture(GL_TEXTURE0);			glBindTexture(GL_TEXTURE_2D, TexAudio);
		glUniformMatrix4fv(progland->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(progland->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glBindVertexArray(VAOland);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBland);		
		offvec = vec3(0, 0, zoffset);	
		glUniform3fv(progland->getUniform("offset"), 1, &offvec.x);			
		screen_offset = vec3(0, 0, 0.001);
		glUniform3fv(progland->getUniform("screen_offset"), 1, &screen_offset.x);
		M = mat4(1);// translate(mat4(1), vec3(0, 0, -ii * CITYMULSCALE * CITYSIZE))* glm::rotate(glm::mat4(1), (float)(3.1415926 * 0.5), glm::vec3(1, 0, 0))* glm::scale(glm::mat4(1), glm::vec3(CITYSIZE * CITYMULSCALE * 0.5, CITYSIZE * CITYMULSCALE * 0.5, 1));
		M = translate(mat4(1), vec3(0.5,-4 , zoffset + camstartoffset));
		glUniformMatrix4fv(progland->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glDrawElements(GL_LINES, IBlandsize, GL_UNSIGNED_INT, (void*)0);			
		progland->unbind();	
				
		glFrontFace(GL_CCW);
		//**************** spheres *********************
		progspheres->bind();
		glActiveTexture(GL_TEXTURE0);			glBindTexture(GL_TEXTURE_2D, TexAudio);
		glUniform1f(progspheres->getUniform("reflection_on"), 0.0);
		glUniformMatrix4fv(progspheres->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(progspheres->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniform3fv(progspheres->getUniform("campos"), 1, &mycam.pos.x);		
				
		glUniform1f(progspheres->getUniform("colormode"), 0);
		glUniform1fv(progspheres->getUniform("freq_ampl"), 10, amplitude_on_frequency_10steps);
		//cout << amplitude_on_frequency_10steps[3] << endl;
		float offset_z = 0;
		int parts = (int)mycam.pos.z / (SPHEREDISTANCEZ*2);
		offset_z = parts * (SPHEREDISTANCEZ * 2);
		for (int ii = 0; ii < 2; ii++)
			{
			M = translate(mat4(1), vec3(0.5, -4, offset_z- SPHEREDISTANCEZ*2 * ii+camstartoffset));// -SPHEREDISTANCEZ * ii ));
			glUniformMatrix4fv(progspheres->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			sphere_land->draw(0, progspheres, true);	//draw sphere
			}				
		//**************** ENDE spheres *********************


		//**************** rects *********************
		glFrontFace(GL_CW);
		
		glUniform1f(progspheres->getUniform("colormode"), 1);
		for (int ii = 0; ii < 2; ii++)
			{
			M = translate(mat4(1), vec3(0.5, -4, offset_z - SPHEREDISTANCEZ * 2 * ii + camstartoffset));// -SPHEREDISTANCEZ * ii ));
			glUniformMatrix4fv(progspheres->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			rects->draw(0, progspheres, true);	//draw sphere
			}
		progspheres->unbind();
		glFrontFace(GL_CCW);
		//**************** ENDE rects *********************


		//bind shader and copy matrices
		prog->bind();

		glUniform1f(prog->getUniform("reflection_on"), 0.0);

		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniform3fv(prog->getUniform("campos"), 1, &mycam.pos.x);

			//************************** LASER *******************************
		if (initialized_VB_laser_count > 0)
		{
			prog_lasereyes->bind();
			glUniformMatrix4fv(prog_lasereyes->getUniform("P"), 1, GL_FALSE, &P[0][0]);
			glUniformMatrix4fv(prog_lasereyes->getUniform("V"), 1, GL_FALSE, &V[0][0]);
			glActiveTexture(GL_TEXTURE0);			glBindTexture(GL_TEXTURE_2D, TexLaser);
			glActiveTexture(GL_TEXTURE1);			glBindTexture(GL_TEXTURE_2D, TexLaserMask); 
			glBindVertexArray(VAOLasers);
			mat4 M = translate(mat4(1),vec3(-10, 5, -100));
			glUniformMatrix4fv(prog_lasereyes->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			glDrawArrays(GL_POINTS, 0, initialized_VB_laser_count);						
			prog_lasereyes->unbind();
		}
		//*********************************************			
	}

	//************************************************************************************************	
	void render_tunnel(int width, int height, mat4 P, mat4 V, bool pilotrender, double frametime)
	{
		float mul_scale = CITYMULSCALE;

		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Get current frame buffer size.

		glViewport(0, 0, width, height);

		glm::mat4 M, S, T;

		float pih = -3.1415926 / 2.0;
		glm::mat4 Rx = glm::rotate(glm::mat4(1.f), pih, glm::vec3(1, 0, 0));

		//	******		skysphere		******

		glFrontFace(GL_CW);
		glDisable(GL_DEPTH_TEST);

		glm::mat4 RxSun = glm::rotate(glm::mat4(1.f), (float)(3.1415926 * 0.5), glm::vec3(1, 0, 0));
		glm::mat4 RySun = glm::rotate(glm::mat4(1.f), (float)(-3.1415926 * 0.5 + 1.5), glm::vec3(0, 1, 0));
		T = glm::translate(glm::mat4(1.f), glm::vec3(mycam.pos));
		S = glm::scale(glm::mat4(1.f), glm::vec3(100.1, 100.1, 100.1));

		progsky->bind();

		glActiveTexture(GL_TEXTURE0);			glBindTexture(GL_TEXTURE_2D, TextureLandBG);
		glActiveTexture(GL_TEXTURE1);			glBindTexture(GL_TEXTURE_2D, TextureLandBGgod);

		glUniformMatrix4fv(progsky->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(progsky->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		float ww = (2. * 3.1415926) / 8.;

		mat4 MM = T * RySun * S;
		glUniformMatrix4fv(progsky->getUniform("M"), 1, GL_FALSE, &MM[0][0]);
		sphere->draw(prog, true);	//draw sky

		progsky->unbind();

		//****************boden LINES ********************
		glEnable(GL_DEPTH_TEST);

		float zoffset = (float)((int)(mycam.pos.z / (TUNNELSIZE*20.0)));
		zoffset = 0;
					
		if (mycam.pos.z < (-TUNNELSIZE * 50))
			mycam.pos.z = 0;
		vec3 offvec = vec3(0, 0, zoffset);
		glBindVertexArray(VAOtunnel);

		glFrontFace(GL_CCW);
		progtunnel2->bind();
		glActiveTexture(GL_TEXTURE0);			glBindTexture(GL_TEXTURE_2D, TexAudio);
		glUniformMatrix4fv(progtunnel2->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(progtunnel2->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBtunnelfull);
		glUniform3fv(progtunnel2->getUniform("offset"), 1, &offvec.x);
		vec3 screen_offset = vec3(0, 0, 0);
		glUniform3fv(progtunnel2->getUniform("screen_offset"), 1, &screen_offset.x);
		vec3 roundlaserpos[10];
		vec3 roundlasercol[10];
		for (int ii = 0; ii < roundlaser.size(); ii++)
		{ 
			roundlaserpos[ii] = roundlaser[ii].pos; roundlasercol[ii] = roundlaser[ii].col;
		}
		glUniform3fv(progtunnel2->getUniform("roundlaserpos"), 10, &roundlaserpos[0].x);
		glUniform3fv(progtunnel2->getUniform("roundlasercol"), 10, &roundlasercol[0].x);
		glUniform1f(progtunnel2->getUniform("roundlasercount"), (float)roundlaser.size() +0.001);
		glUniform3fv(progtunnel2->getUniform("campos"), 1, &mycam.pos.x);			
					
		M = translate(mat4(1), vec3(0.0, 0, zoffset));
		glUniformMatrix4fv(progtunnel2->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glDrawElements(GL_TRIANGLES, IBtunnelfullsize, GL_UNSIGNED_INT, (void*)0);
		if (mycam.pos.z < (-TUNNELSIZE * 40))
		{
			M = translate(mat4(1), vec3(0.0, 0, -TUNNELSIZE * 50));
			glUniformMatrix4fv(progtunnel2->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			glDrawElements(GL_TRIANGLES, IBtunnelfullsize, GL_UNSIGNED_INT, (void*)0);
		}
		progtunnel2->unbind();

		progtunnel->bind();
		glActiveTexture(GL_TEXTURE0);			glBindTexture(GL_TEXTURE_2D, TexAudio);
		glUniformMatrix4fv(progtunnel->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(progtunnel->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBtunnel);
		offvec = vec3(0, 0, zoffset);
		glUniform3fv(progtunnel->getUniform("offset"), 1, &offvec.x);
		screen_offset = vec3(0, 0, 0.001);
		glUniform3fv(progtunnel->getUniform("screen_offset"), 1, &screen_offset.x);
		M = mat4(1);// translate(mat4(1), vec3(0, 0, -ii * CITYMULSCALE * CITYSIZE))* glm::rotate(glm::mat4(1), (float)(3.1415926 * 0.5), glm::vec3(1, 0, 0))* glm::scale(glm::mat4(1), glm::vec3(CITYSIZE * CITYMULSCALE * 0.5, CITYSIZE * CITYMULSCALE * 0.5, 1));
		M = translate(mat4(1), vec3(0.0, 0, zoffset));
		glUniformMatrix4fv(progtunnel->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glDrawElements(GL_LINES, IBtunnelsize, GL_UNSIGNED_INT, (void*)0);
		if (mycam.pos.z < (-TUNNELSIZE * 40))
		{
			M = translate(mat4(1), vec3(0.0, 0, -TUNNELSIZE * 50));
			glUniformMatrix4fv(progtunnel->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			glDrawElements(GL_LINES, IBtunnelsize, GL_UNSIGNED_INT, (void*)0);
		}
		progtunnel->unbind();					
					
		glFrontFace(GL_CCW);
		glFrontFace(GL_CCW);
		//**************** spheres *********************
		progspheres_tunnel->bind();
		glActiveTexture(GL_TEXTURE0);			glBindTexture(GL_TEXTURE_2D, TexAudio);
		glUniform1f(progspheres_tunnel->getUniform("reflection_on"), 0.0);
		glUniformMatrix4fv(progspheres_tunnel->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(progspheres_tunnel->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniform3fv(progspheres_tunnel->getUniform("campos"), 1, &mycam.pos.x);

		glUniform1fv(progspheres_tunnel->getUniform("freq_ampl"), 10, amplitude_on_frequency_10steps);
		//cout << amplitude_on_frequency_10steps[3] << endl;
		float offset_z = 0;
		int parts = (int)mycam.pos.z / (TUNNELSIZE * 10);
		offset_z = parts * (TUNNELSIZE * 10);
		for (int ii = 0; ii < 2; ii++)
		{
			M = translate(mat4(1), vec3(0.5, -4, offset_z - TUNNELSIZE * 10 * ii));// -SPHEREDISTANCEZ * ii ));
			glUniformMatrix4fv(progspheres_tunnel->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			sphere_tunnel->draw(0, progspheres_tunnel, true);	//draw sphere
		}
		progspheres_tunnel->unbind();
		//bind shader and copy matrices
					
		// ****************** roundlaser  ******************

		glEnable(GL_CULL_FACE);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glBindVertexArray(VAOtunnellaser);
		prog_roundlaser->bind();
		glUniformMatrix4fv(prog_roundlaser->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(prog_roundlaser->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glActiveTexture(GL_TEXTURE0);			glBindTexture(GL_TEXTURE_2D, TexTunnelLaser);

		for (int ii = 0; ii < roundlaser.size(); ii++)
		{
			M = translate(mat4(1), roundlaser[ii].pos) * glm::scale(glm::mat4(1), glm::vec3(3.3, 3.3, 1));
			glUniformMatrix4fv(prog_roundlaser->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			vec3 extcolor = roundlaser[ii].col;
			glUniform3fv(prog_roundlaser->getUniform("extcolor"), 1, &extcolor.x);
			glDrawElements(GL_TRIANGLES, IBtunnellasersize, GL_UNSIGNED_INT, (void*)0);
		}
		prog_roundlaser->unbind();
	}

			//*****************************************************************************************
	void get_resolution(int* width, int* height)
	{
		if(!forceresolution)
			glfwGetFramebufferSize(windowManager->getHandle(), width, height);
		else
		{
			*width = FORCERESX;
			*height = FORCERESY;
		}
	}	

	//****************************************************************************************
	void render_scene(int width, int height, mat4 P, mat4 V, bool pilotrender, double frametime)
	{
		switch (rendermode)
		{
			default:
			case MODE_CITYSTATIC:
			case MODE_CITYFWD:
				render_citypilot(width, height, P, M_wobble * V, pilotrender, frametime);
			break;
			case MODE_LANDSTATIC:
			case MODE_LANDFWD:
				render_land(width, height, P,  V, true, frametime);
			break;
			case MODE_TUNNEL:				
				render_tunnel(width, height, P, M_wobble * V, true, frametime);
			break;
			case MODE_BODYSENSE_STATIC:
				break;
		}
	}

	//****************************************************************************************
	void render_to_texture(double frametime) // aka render to framebuffer
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2 ,GL_COLOR_ATTACHMENT3,GL_COLOR_ATTACHMENT4,GL_COLOR_ATTACHMENT5 };
		glDrawBuffers(6, buffers);
		int width, height;
		get_resolution(&width, &height);
		glm::mat4 V, P;
		P = glm::perspective((float)(3.14159 / 3.), (float)((float)width / (float)height), 0.1f, 1000.0f); //so much type casting... GLM metods are quite funny ones
		V = mycam.process(frametime,rendermode);

		render_scene(width * antialiasing, height * antialiasing, P, M_wobble * V, true, frametime);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, FBOcolor);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOviewpos);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOworldpos);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOworldnormal);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBOgodrays);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, FBObloommask);
		glGenerateMipmap(GL_TEXTURE_2D);			
	}

	//*****************************************************************************************
	void render_to_cubemap(double frametime)
	{
		if (!mycam.toggleview) return;
		if (rendermode != MODE_CITYFWD) return;
		//Draw the cubemap.
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Cube_framebuffer);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, Cube_depthbuffer);

		glm::mat4 cubeView[6];
		vec3 campos = mycam.pos;
		/*campos.x *= -1;
		campos.y *= -1;
		campos.z *= -1;*/
		cubeView[0] = glm::lookAt(campos, campos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)); // +X
		cubeView[1] = glm::lookAt(campos, campos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)); // -X
		cubeView[2] = glm::lookAt(campos, campos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // +Y
		cubeView[3] = glm::lookAt(campos, campos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)); // -Y
		cubeView[4] = glm::lookAt(campos, campos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));// +Z
		cubeView[5] = glm::lookAt(campos, campos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)); // -Z

		glm::mat4 cubeProj = glm::perspective(glm::radians(90.0f), (float)1.0f, (float) 0.5, (float)200.0);

		mat4 T = translate(mat4(1), campos);

		/*cubeView[0] = rotate(mat4(1), (float)3.1415926 * 0.5f, vec3(0, -1, 0)) * T;
		cubeView[1] = rotate(mat4(1), -(float)3.1415926 * 0.5f, vec3(0, -1, 0)) * T;
		cubeView[4] = rotate(mat4(1), (float)0.0, vec3(0, -1, 0)) * T;
		cubeView[5] = rotate(mat4(1), (float)3.1415926, vec3(0, -1, 0))*T;*/

		for (int i = 0; i < 6; ++i)
		{
			mat4 V = cubeView[i];

			DrawFace(i, cubeProj, V, frametime);
		}
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}

	//*****************************************************************************************
	void DrawFace(int iFace, mat4 P, mat4 V, double frametime)
	{
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
								GL_TEXTURE_CUBE_MAP_POSITIVE_X + iFace, g_cubeTexture, 0);

		GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
			printf("Status error: %08x\n", status);

		render_scene(CUBE_TEXTURE_SIZE, CUBE_TEXTURE_SIZE, P, V, false, frametime);

	}

	//*****************************************************************************************
	vec2 lasercols[10] = { vec2(0.0,0), vec2(1.0,0.0), vec2(0.0,1.0), vec2(0.5,0.5), vec2(0,0.2), vec2(0.7,0), vec2(0,0.7), vec2(0.7,0.2), vec2(0,1), vec2(1,0) };
	void update_laser_temp(float frametime)
	{
		static float weights[10];
		static bool first = true;
		if (first)
		{
			for (int ii = 0; ii < 10; ii++)
				weights[ii] = 1. + pow((float)(ii / 10.0), 2)*15.;
			first = false;
		}
		vec3 verpos[MAXLASERS];
		vec3 vercol[MAXLASERS];
		vec3 verdir[MAXLASERS];
		if (rendermode == MODE_CITYFWD || rendermode == MODE_CITYSTATIC)
		{
				
			//aging
			for (int i = 0; i < active_lasers.size(); i++)
			{
				active_lasers[i].aging(frametime);
				if (active_lasers[i].age < 0)
				{
					laser_reservour_index_stack.push_back(active_lasers[i].reservour_index);
					change(laser_reservour_index_stack[laser_reservour_index_stack.size() - 1], laser_reservour_index_stack[0]);
					active_lasers.erase(active_lasers.begin() + i);
					i--;
				}
			}
			//trigger:
				
			if (laser_reservour_index_stack.size() > 0)
			{
				for (int i = 0; i < 10 && laser_reservour_index_stack.size()>0; i++)
				{
					float trigger_amplitude = (float)(10. - i) / 2.0;
					trigger_amplitude *= LASERTRIGGERMUL;
					int assigncount = rand() % laser_reservour_index_stack.size() / 2;
					if ((amplitude_on_frequency_10steps[i] > trigger_amplitude && (amplitude_on_frequency_10steps[i] < 100.)) || (i == 2 && beattrigger))
					{
						for (int u = 0; u < assigncount && laser_reservour_index_stack.size()>0; u++)
						{
							//	cout << "ampl " << amplitude_on_frequency_10steps[i] << ", " << trigger_amplitude << ", " << beattrigger << endl;
							UINT num = laser_reservour_index_stack.back();
							laser_reservour_index_stack.pop_back();
							laser_ las = laser_reservour[num];
							las.age = 1.3 + frand() * 0.2;
							las.rg_col = lasercols[i];
							las.reservour_index = num;
							active_lasers.push_back(las);
						}
					}
				}
			}
			beattrigger = false;
			//
			sort(active_lasers.begin(), active_lasers.end(), compareInterval);

			if (active_lasers.size() > 0)
			{
					
				initialized_VB_laser_count = 0;
				for (int i = 0; i < min(active_lasers.size(), MAXLASERS); i++)
					if (active_lasers[i].age > 0 && active_lasers[i].age <= 1.5)
					{

						verpos[initialized_VB_laser_count] = active_lasers[i].pos;
						vercol[initialized_VB_laser_count] = vec3(active_lasers[i].age, active_lasers[i].rg_col.x, active_lasers[i].rg_col.y);
						verdir[initialized_VB_laser_count] = vec3(0, 1, 0);
						initialized_VB_laser_count++;
					}
				if (initialized_VB_laser_count > 0)
				{
					//	cout << "fuck " << initialized_VB_laser_count << endl;
					glBindBuffer(GL_ARRAY_BUFFER, VBLasersPos);
					glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * initialized_VB_laser_count, verpos);
					glBindBuffer(GL_ARRAY_BUFFER, VBLasersCol);
					glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * initialized_VB_laser_count, vercol);
					glBindBuffer(GL_ARRAY_BUFFER, VBLasersDir);
					glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * initialized_VB_laser_count, verdir);
					glBindBuffer(GL_ARRAY_BUFFER, 0);
				}
			}
		}
		else if (rendermode == MODE_TUNNEL)
		{

			//aging
			for (int i = 0; i < roundlaser.size(); i++)
			{
				roundlaser[i].pos.z -= frametime * 30.0;
				if ( mycam.pos.z- roundlaser[i].pos.z  >400)
				{
					roundlaser.erase(roundlaser.begin() + i);
					i--;
				}
			}
			//trigger roundlaser:
			static float oldvals[100] = { 0.0 };
			float average_old = 0, average_new = 0;
			float newval = 0;
			static float lasercooldowntime = 0.0;

			for (int i = 0; i < 10; i++)
				newval += amplitude_on_frequency_10steps[i] * weights[i];
					
			for (int u = 0; u < 100; u++)
			{
				if (u < (100 - 1))		oldvals[u] = oldvals[u + 1];
				else oldvals[u] = newval;

				if (u < 80)		average_old += oldvals[u];
				else			average_new += oldvals[u];
			}
			average_new += newval;
			average_old /= 80.;
			average_new /= 20.;

			float diff = average_new - average_old;
			if (diff > LASERTHRESHOLD+10)
				lasertrigger = true;
				
			if (lasertrigger && lasercooldowntime <= 0.0 && roundlaser.size()<10)
			{
				laser_ las;
				las.pos = vec3(0, 0, mycam.pos.z+100.);
				las.col = normalize(vec3(frand() + 0.001, frand() + 0.001, frand() + 0.001));
				roundlaser.push_back(las);
				lasercooldowntime = 6.0;
			}
			else
				lasertrigger = false;
			if (lasercooldowntime >= 0.0)
				lasercooldowntime -= frametime;

		}
		else if (rendermode == MODE_LANDFWD || rendermode == MODE_LANDSTATIC)
		{
				
			for (int i = 0; i < eyes_lasers.size(); i++)
				if (eyes_lasers[i].age <= 1.5)
					eyes_lasers[i].aging_last(frametime);
				
			//trigger eyeslaser:
			static float oldvals[100] = { 0.0 };
			float average_old=0, average_new=0;
			float newval = 0;
			static float lasercooldowntime = 0.0;
				
			for (int i = 0; i < 10; i++)
			{
				newval += amplitude_on_frequency_10steps[i]* weights[i];
			}
				
				

			for (int u = 0; u < 100; u++)
			{
				if(u<(100 -1))		oldvals[u] = oldvals[u + 1];
				else oldvals[u] = newval;

				if (u < 80)		average_old += oldvals[u];
				else			average_new += oldvals[u];
			}
			average_new += newval;
			average_old /= 80.;
			average_new /= 20.;

			float diff = average_new - average_old;
			if (diff > LASERTHRESHOLD)
			{
					
				lasertrigger = true;
			}
		//	cout << diff << "\t\t" << average_new << "\t\t" << average_old << endl;
				
				
			if (lasertrigger && initialized_VB_laser_count <= 0 && lasercooldowntime <=0.0)
			{
				for (int i = 0; i < eyes_lasers.size(); i++)
				{
					eyes_lasers[i].age = 1.4;
					eyes_lasers[i].rg_col = vec2(1, 0.3);
				}
			lasercooldowntime = 16.0;
			}
			if(lasercooldowntime >=0.0)
				lasercooldowntime -= frametime;

			if (eyes_lasers.size() > 0)
			{
				
				initialized_VB_laser_count = 0;
				for (int i = 0; i < eyes_lasers.size(); i++)
				{
					if (eyes_lasers[i].age > 0)
					{
						vec4 vpos = vec4(eyes_lasers[i].pos, 1);
						vpos = M_facetrinity * vpos;
						verpos[initialized_VB_laser_count] = vec3(vpos);
						vercol[initialized_VB_laser_count] = vec3(eyes_lasers[i].age, eyes_lasers[i].rg_col.x, eyes_lasers[i].rg_col.y);
						verdir[initialized_VB_laser_count] = eyes_lasers[i].direction;
						initialized_VB_laser_count++;
					}
				}
				if (initialized_VB_laser_count > 0)
				{
					//	cout << "fuck " << initialized_VB_laser_count << endl;
					glBindBuffer(GL_ARRAY_BUFFER, VBLasersPos);
					glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * initialized_VB_laser_count, verpos);
					glBindBuffer(GL_ARRAY_BUFFER, VBLasersCol);
					glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * initialized_VB_laser_count, vercol);
					glBindBuffer(GL_ARRAY_BUFFER, VBLasersDir);
					glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3)* initialized_VB_laser_count, verdir);
					glBindBuffer(GL_ARRAY_BUFFER, 0);
				}
				else
					lasertrigger = false;
		
			}
		}
		beattrigger = false;
	}
	//*********************************************************************************************************
	void init_laser()
	{
		int cc = 0;
		for (int xx = -CITYSLOTS; xx < CITYSLOTS; xx++)
		{
			for (int zz = -CITYSLOTS; zz < CITYSLOTS; zz++)
			{
				if (cityfield[xx + CITYSLOTS][zz + CITYSLOTS].height > 4.5)
				{
					laser_ las;
					las.pos = vec3(xx * BULDINGDIST, 2, zz * BULDINGDIST) * (float)CITYMULSCALE;

					//	las.pos = vec3(-3,0, -cc *5);
					las.fieldpos = ivec2(xx, zz);
					laser_reservour_index_stack.push_back(cc);
					las.reservour_index = cc++;
					laser_reservour.push_back(las);
					if (laser_reservour.size() >= MAXLASERS)
					{
						xx = CITYSLOTS;
						break;
					}

				}
			}
		}
		shuffle_lasers(laser_reservour.size());

		for (int i = 0; i < 10; i++)
		{
			amplitude_on_frequency_10steps[i] = 0;
		}
		//laser for the eyes:
		laser_ las;
		las.age = 0.0;
		las.direction = vec3(0, 0, 1);
		las.pos = vec3(-0.2, 0.0, -0.6);
		eyes_lasers.push_back(las);
		las.pos = vec3(0.33, 0.0, -0.6);
		eyes_lasers.push_back(las);

		las.pos = vec3(-1.2 + 0.06, 0.0, 0.2);
		eyes_lasers.push_back(las);
		las.pos = vec3(-0.8 + 0.06, 0.0, -0.1);
		eyes_lasers.push_back(las);
			
		las.pos = vec3(1.2, 0.0, 0.2);
		eyes_lasers.push_back(las);
		las.pos = vec3(0.8, 0.0, -0.1);
		eyes_lasers.push_back(las);

	}

	void texturetest_render_to_framebuffer()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, texturetestfb);


		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		double frametime = get_last_elapsed_time();

		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Create the matrix stacks - please leave these alone for now

		glm::mat4 V, M, P; //View, Model and Perspective matrix
		V = mycam.process(frametime, MODE_TEXTURE_TEST);
		M = glm::mat4(1);
		// Apply orthographic projection....
		P = glm::ortho(-1 * aspect, 1 * aspect, -1.0f, 1.0f, -2.0f, 100.0f);
		if (width < height)
		{
			P = glm::ortho(-1.0f, 1.0f, -1.0f / aspect, 1.0f / aspect, -2.0f, 100.0f);
		}
		// ...but we overwrite it (optional) with a perspective projection.
		P = glm::perspective((float)(3.14159 / 4.), (float)((float)width / (float)height), 0.1f, 1000.0f); //so much type casting... GLM metods are quite funny ones
		float sangle = 3.1415926 / 2.;
		glm::mat4 RotateXSky = glm::rotate(glm::mat4(1.0f), sangle, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec3 camp = -mycam.pos;
		glm::mat4 TransSky = glm::translate(glm::mat4(1.0f), camp);
		glm::mat4 SSky = glm::scale(glm::mat4(1.0f), glm::vec3(0.8f, 0.8f, 0.8f));

		M = TransSky * RotateXSky * SSky;

		// Draw the box using GLSL.
		ptexturetestsky->bind();


		//send the matrices to the shaders
		glUniformMatrix4fv(ptexturetestsky->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(ptexturetestsky->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(ptexturetestsky->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(ptexturetestsky->getUniform("campos"), 1, &mycam.pos[0]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texturetestTexture2);
		glDisable(GL_DEPTH_TEST);
		texturetest_skysphere->draw(ptexturetestsky, FALSE);
		glEnable(GL_DEPTH_TEST);

		ptexturetestsky->unbind();

		//animation with the model matrix:
		static float w = 0.0;
		w += 1.0 * frametime;//rotation angle
		float trans = 0;// sin(t) * 2;
		glm::mat4 RotateY = glm::rotate(glm::mat4(1.0f), w, glm::vec3(0.0f, 1.0f, 0.0f));
		float angle = -3.1415926 / 2.0;
		glm::mat4 RotateX = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 TransZ = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3 + trans));
		glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(0.8f, 0.8f, 0.8f));

		M = TransZ * RotateY * RotateX * S;

		// Draw the box using GLSL.
		ptexturetest1->bind();


		//send the matrices to the shaders
		glUniformMatrix4fv(ptexturetest1->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(ptexturetest1->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(ptexturetest1->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(ptexturetest1->getUniform("campos"), 1, &mycam.pos[0]);

		glBindVertexArray(texturetestVertexArrayID);
		//actually draw from vertex 0, 3 vertices
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, texturetestIndexBufferIDBox);
		//glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (void*)0);
		mat4 Vi = glm::transpose(V);
		Vi[0][3] = 0;
		Vi[1][3] = 0;
		Vi[2][3] = 0;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texturetestTexture);

		M = TransZ * S * Vi;
		glUniformMatrix4fv(ptexturetest1->getUniform("M"), 1, GL_FALSE, &M[0][0]);

		glDisable(GL_DEPTH_TEST);
		glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0, 500);
		glEnable(GL_DEPTH_TEST);
		glBindVertexArray(0);

		ptexturetest1->unbind();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, texturetestFBOtex);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	void texturetest_render()
	{
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);
		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		ptexturetestpostproc->bind();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texturetestFBOtex);
		glBindVertexArray(texturetestVertexArrayIDScreen);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		ptexturetestpostproc->unbind();
	}
};
//*********************************************************************************************************
void modechange(double frametime)
{
	if (!mycam.toggle_auto)			return;
	if (!(rendermode == MODE_CITYFWD || rendermode == MODE_CITYSTATIC || rendermode == MODE_TUNNEL || rendermode == MODE_BODYSENSE_STATIC)) return;

	static double totaltime_since_last_change = 0;
	static double seconds_in_reverse_mode = 0;
	static double seconds_since_last_reverse_mode = 0;
	static double seconds_in_helmet_mode = 0;
	static double seconds_since_last_helmet_mode = 0;

	totaltime_since_last_change += frametime;
	//record mode stats:
	if (mycam.toggleview)
	{
		seconds_in_helmet_mode += frametime;
		seconds_since_last_helmet_mode = 0;
	}
	else
	{
		seconds_since_last_helmet_mode += frametime;
	}
	if (mycam.rot.z > 2.0)
	{
		seconds_in_reverse_mode += frametime;
		seconds_since_last_reverse_mode = 0;
	}
	else
		seconds_since_last_reverse_mode += frametime;
	//consequences:
	bool dont_helmet = false;
	bool dont_inverse = false;
	if (seconds_in_helmet_mode > 10.0 || rendermode == MODE_TUNNEL)
	{
		dont_helmet = true;
		mycam.toggleview = false;
	}
	if (seconds_since_last_helmet_mode > 35)
	{
		seconds_in_helmet_mode = 0;
		dont_helmet = false;
	}

	if (seconds_in_reverse_mode > 20.0)
	{
		dont_inverse = true;
		mycam.rot.z = 0;
	}
	if (seconds_since_last_reverse_mode > 35)
	{
		seconds_in_reverse_mode = 0;
		dont_inverse = false;
	}
	//chance for a change
	if (totaltime_since_last_change > CHANGEMODETIME)
	{
		float r = frand();
		if (r < 0.1) mycam.toggle();
		else if (r > 0.9)
		{
			if (mycam.rot.z < 1)
			{
				mycam.rot.z = 3.1415926;
			}
			else
			{
				mycam.rot.z = 0;
			}
		}
		totaltime_since_last_change = 0;
	}
}
//******************************



//********************************************************************************
extern int running;
int main(int argc, char** argv)
{
	 float weights[TEXELSIZE];
	
	for (int ii = 0; ii < TEXELSIZE; ii++)
	{			
		weights[ii] = 1. + pow((float)(ii / 10.0), 2)*10.;
	}

	srand(time(0));
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application* application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager* windowManager = new WindowManager();
	windowManager->init(1280, 720);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;
	application->init(resourceDir);
	application->initGeom(resourceDir);

#ifndef NOAUDIO
	thread t1(start_recording);
#endif
	
	// Loop until the user closes the window.
	int framecount = 0;
	double fps[10] = { 0,0,0,0,0,0,0,0,0,0 };
	StopWatchMicro_ sw;


	while (!glfwWindowShouldClose(windowManager->getHandle()))
	{
		static double totaltime = 0;
		double frametime = get_last_elapsed_time();
		totaltime += frametime;
		framecount++;

		windowManager->SetFullScreen(fullscreen);

		modechange(frametime);

		/*fps[0] += frametime;
		if (framecount > 100 && frametime > 0)
			{
			cout << "fps all: " << (double)framecount / fps[0] << endl;
			cout << "mu s 1: " << fps[1] / 100. << endl;
			cout << "mu s 2: " << fps[2] / 100. << endl;
			cout << "mu s 3: " << fps[3] / 100. << endl;
			cout << "mu s 4: " << fps[4] / 100. << endl;
			cout << "mu s 5: " << fps[5] / 100. << endl;
			cout << "mu s 6: " << fps[6] / 100. << endl;
			cout << "mu s 7: " << fps[7] / 100. << endl;
			framecount = 0;
			for (int ii = 0; ii < 10; ii++)	fps[ii] = 0;
			}*/
			// Render scene.
			//Set the FFT arrays
		//	sw.start();
		application->update_laser_temp(frametime);
		//	fps[1] += sw.elapse_micro();
		//	sw.start();
		application->update_wobble_pilot(frametime);
		//fps[2] += sw.elapse_micro();
		//sw.start();
		#ifndef NOAUDIO
			application->aquire_fft_scaling_arrays();
			
		#endif	
		//fps[3] += sw.elapse_micro();
	//	sw.start();
		application->render_to_cubemap(frametime);
		//	fps[4] += sw.elapse_micro();
			//sw.start();
		application->render_to_texture(frametime);
		//	fps[5] += sw.elapse_micro();
		//	sw.start();
		application->render_merge_to_postproc();
		//	fps[6] += sw.elapse_micro();
		//	sw.start();
		bool bluron = false;
		bool bloomon = false;
		if (mycam.toggleview && rendermode == MODE_CITYFWD)
			bluron = true;
		if (rendermode == MODE_LANDFWD || rendermode == MODE_TUNNEL)
			bluron = false;
		if (rendermode == MODE_LANDFWD || rendermode == MODE_TUNNEL)
			bloomon = true;
		
		if (bloomon && (rendermode == MODE_LANDFWD || rendermode == MODE_TUNNEL))
		{
			application->render_postproc(frametime, 0, bluron, application->prog_bloom,true, application->FBOcolor_no_ssaa[1], application->FBOcolor_no_ssaa[0], application->FBOcolor_bloommap);
			application->render_postproc(frametime, 1, bluron, application->prog_bloom, false, NULL,application->FBOcolor_no_ssaa[0], application->FBOcolor_no_ssaa[1]);
		}
		else if(rendermode == MODE_BODYSENSE_STATIC)
		{
			application->prepare_to_render();
			application->render();
		}
		else if (rendermode == MODE_TEXTURE_TEST)
		{
			application->texturetest_render_to_framebuffer();
			application->texturetest_render();
		}
		else
		{
			application->render_postproc(frametime, 0, bluron, application->prog_blur, true, application->FBOcolor_no_ssaa[1], application->FBOcolor_no_ssaa[0], application->FBOcolor_blurmap);
			application->render_postproc(frametime, 1, bluron, application->prog_blur, false, NULL, application->FBOcolor_no_ssaa[1],application->FBOcolor_blurmap);
		}
		//	fps[7] += sw.elapse_micro();

			// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}
	running = FALSE;
#ifndef NOAUDIO
	t1.join();
#endif
	
	// Quit program.
	windowManager->shutdown();
	return 0;
}
