#include "MyMeshBuilder.h"
#include <math.h>
#include <iostream>
#include "Program.h"
#include "stb_image.h"

/*void Revolution(int density, int circDens, mat4(*M)(float), vector<GLfloat> *points, vector<GLfloat> *norms, vector<GLuint> *indices) {
	float t = 0, a = 0, da = (2. * PI) / (float) circDens, tInc = 1. / (float)density;
	mat4 m;
	vec3 point, PointA, PointB, PointC;
	vec3 norm;
	int A, B, C, temp;
	for (int i = 0; i <= density; ++i, t += tInc) {
		t = (i == density) ? 1. : t;
		m = M(t);
		a = 0.;
		for (int j = 0; j < circDens; ++j, a += da) {
			point = vec3(cos(a), 0., sin(a));
			point = m * vec4(point, 1.);
			pushBack3f(points, &point);
		}
	}

	for (int i = 0; i < density; ++i) {
		for (int j = 0; j < circDens; ++j) {
			C = (B = (A = (j + (i * circDens))) + circDens) + 1;
			C = (C == (i + 2) * circDens) ? (i + 1) * circDens : C;
			pushBack3i(indices, A, B, C);

			temp = (C == (B + 1)) ? A + 1 : (A + 1 - circDens);
			pushBack3i(indices, C, temp, A);
			
		}
	}


}*/

int initObject(vector<GLfloat> *points, vector<GLfloat> *normals, vector<GLuint> *indices, GLuint *vaID, GLuint *vbID, GLuint *vcID, GLuint *ibID) {

	//generate the VAO
	glGenVertexArrays(1, vaID);
	glBindVertexArray(*vaID);
	//generate vertex buffer to hand off to OGL
	glGenBuffers(1, vbID);
	//set the current state to focus on our vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, *vbID);
	//actually memcopy the data - only do this once
	glBufferData(GL_ARRAY_BUFFER, points->size() * sizeof(float), &(*(points))[0], GL_DYNAMIC_DRAW);
	//we need to set up the vertex array
	glEnableVertexAttribArray(0);
	//key function to get up how many elements to pull out at a time (3)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glGenBuffers(1, vcID);
	//set the current state to focus on our vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, *vcID);
	glBufferData(GL_ARRAY_BUFFER, normals->size() * sizeof(float), &(*(normals))[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glGenBuffers(1, ibID);
	//set the current state to focus on our vertex buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ibID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices->size(), &(*(indices))[0], GL_STATIC_DRAW);
	glBindVertexArray(0);
	return indices->size();
}

void square(int density, vector<float> * points, vector<float> * texts, vector<uint> * inds) {
	if (density % 2) ++density;
	float t = 1.f / density;
	for (int x = -density / 2; x < density / 2; ++x) {
		for (int y = -density / 2; y < density / 2; ++y) {
			pushBack3f(points, &(vec3(0.0, 0.0, 0.0) + vec3(x, 0, y)));
			pushBack3f(points, &(vec3(1.0, 0.0, 0.0) + vec3(x, 0, y)));
			pushBack3f(points, &(vec3(1.0, 0.0, 1.0) + vec3(x, 0, y)));
			pushBack3f(points, &(vec3(0.0, 0.0, 1.0) + vec3(x, 0, y)));
			pushBack2f(texts, &(vec2(0.0, 0.0) + vec2(x, y)*t));
			pushBack2f(texts, &(vec2(t, 0.0) + vec2(x, y)*t));
			pushBack2f(texts, &(vec2(t, t) + vec2(x, y)*t));
			pushBack2f(texts, &(vec2(0.0, t) + vec2(x, y)*t));
		}
	}
	int ind = 0;
	for (int i = 0; i< density * density * 6; i += 6, ind += 4) {
		inds->push_back(ind + 0);
		inds->push_back(ind + 1);
		inds->push_back(ind + 2);
		inds->push_back(ind + 0);
		inds->push_back(ind + 2);
		inds->push_back(ind + 3);
	}
}

void isl(int density, vector<float> * points, vector<float> * texts, vector<uint> * inds) {
	float t = 1.f / density;
	for (int x = 0; x < density; ++x) {
		for (int y = 0; y < density; ++y) {
			float z = (-((float)(y * y) / (density * density)) * 10) + 1;
			pushBack3f(points, &(vec3(0.0, 0.0, 0.0) + vec3(x, z, y)));
			pushBack3f(points, &(vec3(1.0, 0.0, 0.0) + vec3(x, z, y)));
			pushBack3f(points, &(vec3(1.0, 0.0, 1.0) + vec3(x, z, y)));
			pushBack3f(points, &(vec3(0.0, 0.0, 1.0) + vec3(x, z, y)));
			pushBack2f(texts, &(vec2(0.0, 0.0) + vec2(x, y)*t));
			pushBack2f(texts, &(vec2(t, 0.0) + vec2(x, y)*t));
			pushBack2f(texts, &(vec2(t, t) + vec2(x, y)*t));
			pushBack2f(texts, &(vec2(0.0, t) + vec2(x, y)*t));
		}
	}
	int ind = 0;
	for (int i = 0; i< density * density * 6; i += 6, ind += 4) {
		inds->push_back(ind + 0);
		inds->push_back(ind + 1);
		inds->push_back(ind + 2);
		inds->push_back(ind + 0);
		inds->push_back(ind + 2);
		inds->push_back(ind + 3);
	}
}

void Rain::makeRain(char *filepath) {
	//generate the VAO
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	//generate vertex buffer to hand off to OGL
	glGenBuffers(1, &VertexBufferID);
	//set the current state to focus on our vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID);

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
	glGenBuffers(1, &VertexNormDBox);
	//set the current state to focus on our vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, VertexNormDBox);
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
	glGenBuffers(1, &VertexTexBox);
	//set the current state to focus on our vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, VertexTexBox);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_tex), cube_tex, GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glGenBuffers(1, &IndexBufferIDBox);
	//set the current state to focus on our vertex buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferIDBox);
	GLushort cube_elements[] = {

		// front
		0, 1, 2,
		2, 3, 0,
	};
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_elements), cube_elements, GL_STATIC_DRAW);


	//generate vertex buffer to hand off to OGL ###########################
	glGenBuffers(1, &InstanceBuffer);
	//set the current state to focus on our vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, InstanceBuffer);
	glm::vec4 *positions = new glm::vec4[num];
	for (int i = 0; i < num; i++) {
		float rz = ((float)rand() / ((float)RAND_MAX)) - .5;
		float rx = ((float)rand() / ((float)RAND_MAX)) - .5;
		float ry = ((float)rand() / ((float)RAND_MAX));
		positions[i] = vec4(rx * 100, ry * 100, rz * 100, 0);
	}
	//actually memcopy the data - only do this once
	glBufferData(GL_ARRAY_BUFFER, num * sizeof(glm::vec4), positions, GL_STATIC_DRAW);
	int position_loc = glGetAttribLocation(prog->pid, "InstancePos");
	for (int i = 0; i < num; i++) {
		// Set up the vertex attribute
		glVertexAttribPointer(position_loc + i,              // Location
							  4, GL_FLOAT, GL_FALSE,       // vec4
							  sizeof(vec4),                // Stride
							  (void *)(sizeof(vec4) * i)); // Start offset
														   // Enable it
		glEnableVertexAttribArray(position_loc + i);
		// Make it instanced
		glVertexAttribDivisor(position_loc + i, 1);
	}


	glBindVertexArray(0);



	int width, height, channels;

	//texture 1

	
	unsigned char* data = stbi_load(filepath, &width, &height, &channels, 4);
	glGenTextures(1, &Texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);


	//[TWOTEXTURES]
	//set the 2 textures to the correct samplers in the fragment shader:
	GLuint Tex1Location = glGetUniformLocation(prog->pid, "tex");//tex, tex2... sampler in the fragment shader
	GLuint Tex2Location = glGetUniformLocation(prog->pid, "tex2");
	// Then bind the uniform samplers to texture units:
	glUseProgram(prog->pid);
	glUniform1i(Tex1Location, 0);
	glUniform1i(Tex2Location, 1);
}

void Rain::draw(mat4 P, mat4 M, mat4 V, vec3 campos, float time) {
	prog->bind();
	//send the matrices to the shaders
	glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P[0][0]);
	glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, &V[0][0]);
	glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
	glUniform1f(prog->getUniform("time"), time);
	vec3 camoff = vec3((int)campos.x, 0, (int)campos.z);
	glUniform3fv(prog->getUniform("campos"), 1, &camoff[0]);



	glBindVertexArray(VertexArrayID);
	//actually draw from vertex 0, 3 vertices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferIDBox);
	//glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (void*)0);
	mat4 Vi = glm::transpose(V);
	Vi[0][3] = 0;
	Vi[1][3] = 0;
	Vi[2][3] = 0;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Texture);
	M = M * Vi;
	glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);

	glDisable(GL_DEPTH_TEST);
	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0, num);
	glEnable(GL_DEPTH_TEST);
	glBindVertexArray(0);


	prog->unbind();

}

void bridges(int density, vector<float> * points, vector<float> * texts, vector<uint> * inds) {
	float t = 1.f / density;
	for (int x = 0; x < density; ++x) {
		for (int y = 0; y < density; ++y) {
			pushBack3f(points, &(vec3(0.0, 0.0, 0.0) + vec3(x, 0, y)));
			pushBack3f(points, &(vec3(1.0, 0.0, 0.0) + vec3(x, 0, y)));
			pushBack3f(points, &(vec3(1.0, 0.0, 1.0) + vec3(x, 0, y)));
			pushBack3f(points, &(vec3(0.0, 0.0, 1.0) + vec3(x, 0, y)));
			pushBack2f(texts, &(vec2(0.0, 0.0) + vec2(x, y)*t));
			pushBack2f(texts, &(vec2(t, 0.0) + vec2(x, y)*t));
			pushBack2f(texts, &(vec2(t, t) + vec2(x, y)*t));
			pushBack2f(texts, &(vec2(0.0, t) + vec2(x, y)*t));
		}
	}

	float d2 = (float)density / 2.f;
	for (int i = 0; i < points->size(); i += 3) {
		float y = points->at(i + 2);
		points->at(i + 1) = ((-((y - d2) * (y - d2)) + (d2 * d2)) / (d2 * d2)) * 10;
	}
	int ind = 0;
	for (int i = 0; i< density * density * 6; i += 6, ind += 4) {
		inds->push_back(ind + 0);
		inds->push_back(ind + 1);
		inds->push_back(ind + 2);
		inds->push_back(ind + 0);
		inds->push_back(ind + 2);
		inds->push_back(ind + 3);
	}
}

float hashF(float n) { return fract(sin(n) * 753.5453123); }
float snoise(vec3 x) {
	vec3 p = floor(x);
	vec3 f = fract(x);
	f = f * f * (3.0f - 2.0f * f);

	float n = p.x + p.y * 157.0 + 113.0 * p.z;
	return mix(mix(mix(hashF(n + 0.0), hashF(n + 1.0), f.x),
			   mix(hashF(n + 157.0), hashF(n + 158.0), f.x), f.y),
			   mix(mix(hashF(n + 113.0), hashF(n + 114.0), f.x),
			   mix(hashF(n + 270.0), hashF(n + 271.0), f.x), f.y), f.z);
}
//Changing octaves, frequency and presistance results in a total different landscape.
float noise(vec3 position, int octaves, float frequency, float persistence) {
	float total = 0.0;
	float maxAmplitude = 0.0;
	float amplitude = 1.0;
	for (int i = 0; i < octaves; i++) {
		total += snoise(position * frequency) * amplitude;
		frequency *= 2.0;
		maxAmplitude += amplitude;
		amplitude *= persistence;
	}
	return total / maxAmplitude;
}

void makeTerrain(vector<float> *pts) {
	/*for (int i = 0; i < pts->size(); i += 3) {
		vec3 pos = vec3(pts->at(i), pts->at(i + 1), pts->at(i + 2));
		float height = noise(pos, 11, .03, .05);
		float baseheight = noise(pos, 4, 0.004, 0.3);
		baseheight = pow(baseheight, 2) * 3;
		height = baseheight*height;
		pts->at(i + 1) += height;
	}*/
}

void normals(vector<float> *normBuf, vector<float> *pts, vector<uint> *eleBuf) {
	vector<float> posBuf;

	// shiny dragon
	for (int i = 0; i < eleBuf->size(); ++i) 
		for (int j = 0; j < 3; ++j) 
			posBuf.push_back(pts->at((3 * eleBuf->at(i)) + j));
		
	for (int i = 0; i < posBuf.size(); i += 9) {
		vec3 tri[3];
		for (int j = 0; j < 3; ++j) tri[j] = vec3(posBuf.at(i + (j * 3)), posBuf.at(i + (j * 3) + 1), posBuf.at(i + (j * 3) + 2));
		vec3 prod = cross((tri[1] - tri[0]), (tri[2] - tri[0]));
		for (int k = 0; k < 3; ++k) {
			normBuf->push_back(prod.x);
			normBuf->push_back(prod.y);
			normBuf->push_back(prod.z);
		}
	}
}
mat4 ZERO(float t) {
	return translate(mat4(1.0f), vec3(0., (t * 2.) -1., 0.));
}

mat4 cylinder(float t) {
	return ZERO(t) * scale(mat4(1.0), vec3(1.0, 1.0, 1.0));
}

mat4 cylinder2(float t) {
	return ZERO(t / 2.) * scale(mat4(1.0), vec3(1.0, 1.0, 1.0));
}

mat4 sphere(float t) {
	float y = (t * 2) - 1;
	float rad = cos(asin(y));
	return ZERO(t) * scale(mat4(1.0f), vec3(rad, 1., rad)); // cos(asin(y));
}

mat4 cone(float t) {
	float y = (t * 2) - 1;
	float rad = ((.5 * y) + .5);
	return ZERO(t) * scale(mat4(1.0f), vec3(rad, 1., rad));
}

mat4 cone2(float t) {
	t = (t / 2.) + 1.;
	float y = (t * 2) - 1;
	float rad = ((.5 * y) + .5);
	return ZERO(t) * scale(mat4(1.0f), vec3(rad, 1., rad));
}

/*mat4 handleRot(float t) {
	float w = 
	return vec3(cos(asin(y)) * .5, 0, y);
}*/

mat4 saddle(float t) {
	float rad, y = (2 * t) - 1;
	rad = (y * y) + .2;
	return ZERO(t) * scale(mat4(1.0f), vec3(rad, 1., rad));
}


mat4 coil(float t) {
	float a = t * 2. * PI * 2.5; // 2.5 = num Spirals
	float x = cos(a), z = sin(a), y = (t * 2.) - 1;
	float w = PI / 2.;
	return translate(mat4(1.0f), vec3(x, y, z)) * rotate(mat4(1.0f), w, vec3(x, y, z)) * scale(mat4(1.0f), vec3(0.1, 1., 0.1));
}

mat4 coil2(float t) {
	float a = t * 2. * PI * 2.5; // 2.5 = num Spirals
	float x = cos(a), z = sin(a), y = (t * 2.) - 1;
	x *= t;
	z *= t;
	float w = PI / 2.;
	return translate(mat4(1.0f), vec3(x, y, z)) * rotate(mat4(1.0f), w, vec3(x, y, z)) * scale(mat4(1.0f), vec3(0.1, 1., 0.1));
}

mat4 ring(float t) {
	float a = t * 2. * PI * 1.001; // 2.5 = num Spirals
	float x = cos(a), z = sin(a), y = 0;;
	float w = PI / 2.;
	return translate(mat4(1.0f), vec3(x, y, z)) * rotate(mat4(1.0f), w, vec3(x, y, z)) * scale(mat4(1.0f), vec3(0.1, 1., 0.1));
}

mat4 weirdRing(float t) {
	float x = cos(t * 2 * PI), y = sin(t * 2 * PI), z = 0, w = PI / 2.;
	//y = sin(x = t * PI);
	float s = (-pow(((2. * t) - 1.), 2.) + 2.) / 2.;
	mat4 T = translate(mat4(1.0f), vec3(x, y, z));
	//mat4 S = scale(mat4(1.0f), vec3(s, s, s));
	mat4 R = rotate(mat4(1.0f), w, vec3(x, y, z));
	return T * R;// *S;
}

mat4 tail(float t) {
	float time = (t * 2 * PI);
	float x = time, z = 0, y = sin(time),  w = PI / 2.;
	float s = (-pow(((2. * t) - .75), 2.) + 2.) / 3.;
	mat4 S = scale(mat4(1.0f), vec3(s, s, s));
	mat4 R = rotate(mat4(1.0f), w, vec3(0, 0, 1 - cos(time)));
	mat4 T = translate(mat4(1.0f), vec3(x, y, z));
	return T * R * S;
}


void pushBack3f(vector<GLfloat> *vec, vec3 *v3) {
	vec->push_back(v3->x);
	vec->push_back(v3->y);
	vec->push_back(v3->z);
}

void pushBack2f(vector<GLfloat> *vec, vec2 *v2) {
	vec->push_back(v2->x);
	vec->push_back(v2->y);
}

void pushBack3i(vector<GLuint> *vec, GLuint a, GLuint b, GLuint c) {
	vec->push_back(a);
	vec->push_back(b);
	vec->push_back(c);
}

