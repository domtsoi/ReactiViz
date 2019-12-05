#include "helpers.h"

using namespace glm;
using namespace std;


void genericDraw(shared_ptr<Program> prog, mat4 *M, mat4 *P, mat4*V, camera *mycam) 
{
	glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0][0]);
	glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P[0][0][0]);
	glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, &V[0][0][0]);
	glUniform3fv(prog->getUniform("campos"), 1, &mycam->pos[0]);
}

shared_ptr<Program> initProg(string &vert, string &frag, char** uniforms, char ** attributes, uint us, uint as) 
{
	shared_ptr<Program> prog = make_shared<Program>();

	prog->setVerbose(true);
	prog->setShaderNames(vert, frag);
	if (!prog->init()) {
		std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
		exit(1);
	}
	for (int i = 0; i < us; ++i) {
		prog->addUniform(uniforms[i]);
	}
	for (int i = 0; i < as; ++i) {
		prog->addAttribute(attributes[i]);
	}

	return prog;
}
