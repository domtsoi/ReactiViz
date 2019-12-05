#ifndef __MY__HELPERS__
#define __MY__HELPERS__

#include "../../necessary_includes.h"
#include "../../camera.h"

void genericDraw(std::shared_ptr<Program>, glm::mat4 *, glm::mat4 *, glm::mat4*, camera *);
std::shared_ptr<Program> initProg(std::string &vert, std::string &frag, char** uniforms, char ** attributes, glm::uint us, glm::uint as);

#endif
