#ifndef __NECESSARY_INCLUDES__
#define __NECESSARY_INCLUDES__

#include <iostream>
#include <glad/glad.h>
#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Shape.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

//#define WIDTH (1920)
//#define HEIGHT (1080)
//
//class camera {
//public:
//	glm::vec3 pos, rot = glm::vec3(0);
//	int w, a, s, d, j, n, l, m, k;
//	camera() {
//		w = a = s = d = j = n = l = m = k = 0;
//		pos = glm::vec3(0, 0, 0);
//	}
//	glm::mat4 process(double ftime) {
//		float speed = 0;
//		if (w == 1)
//		{
//			speed = 100*ftime; 
//		}
//		else if (s == 1)
//		{
//			speed = -100*ftime;
//		}
//		float yangle = 0;
//		if (a == 1)
//			yangle = -3 * ftime;
//		else if (d == 1)
//			yangle = 3 * ftime;
//		rot.y += yangle;
//		glm::mat4 R = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
//		glm::vec4 dir = glm::vec4(0, 0, speed, 1);
//		dir = dir*R;
//		pos += glm::vec3(dir.x, dir.y, dir.z);
//		return R *  glm::translate(glm::mat4(1), pos);
//	}
//};

#endif
