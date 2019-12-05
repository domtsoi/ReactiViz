#ifndef __MY_MESH_BUILDER_H
#define __MY_MESH_BUILDER_H

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "GLSL.h"
#include "Program.h"
// include vector class
#include <vector>

// include memory class
#include <memory>

using namespace glm;
using namespace std;


#define PI (3.14159)

void Revolution(int density, int circDens, mat4(*M)(float), vector<GLfloat> *, vector<GLfloat> *, vector<GLuint> *);

void calcNorm(vector<GLfloat> *, vector<GLfloat> *);

int initObject(vector<GLfloat> *, vector<GLfloat> *, vector<GLuint> *, GLuint *, GLuint *, GLuint *, GLuint *);

void normals(vector<float> *normBuf, vector<float> *posBuf, vector<uint> *inds);

void makeTerrain(vector<float> *points);

mat4 ZERO(float t);

/*mat4 handleRot(float t);*/

mat4 cylinder(float t);

mat4 cylinder2(float t);

mat4 sphere(float t);

mat4 cone(float t);

mat4 cone2(float t);

mat4 coil(float t);

mat4 saddle(float t);

mat4 coil2(float t);

mat4 ring(float t);

mat4 weirdRing(float t);

mat4 tail(float t);

void pushBack3f(vector<GLfloat> *vec, vec3 *v3);

void pushBack2f(vector<GLfloat> *vec, vec2 *v2);

void pushBack3i(vector<GLuint> *vec, GLuint a, GLuint b, GLuint c);

void square(int density, vector<float> * points, vector<float> * texts, vector<uint> * inds);

void bridges(int density, vector<float> * points, vector<float> * texts, vector<uint> * inds);

class Rain {
	public :
		Rain() {}
		Rain(int n, shared_ptr<Program> p, char * str) {
			prog = p;
			num = n;
			makeRain(str);
		}
		void draw(mat4 P, mat4 M, mat4 V, vec3 campos, float time);
	private :
		void makeRain(char * filepath);
		shared_ptr<Program> prog;
		GLuint VertexBufferID;
		GLuint VertexArrayID;
		GLuint VertexNormDBox;
		GLuint VertexTexBox;
		GLuint IndexBufferIDBox;
		GLuint InstanceBuffer;
		GLuint Texture;
		int num;
};

#endif
