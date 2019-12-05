#ifndef __BILLBOARD__H__
#define __BILLBOARD__H__

#include "../../necessary_includes.h"
#include "../Helpers/helpers.h"
#include "../../line.h"
#include "../Frames/Frames.h"
#include "../../stb_image.h"

class Billboard 
{
	public:
		Billboard();
		Billboard(std::vector<std::string> *, std::shared_ptr<Program>);
		void draw(shared_ptr<Frames>, shared_ptr<vector<Body>>, shared_ptr<vector<int>>);
		void draw(shared_ptr<Frames>, shared_ptr<vector<int>>, int);
		void draw(glm::mat4 *, glm::mat4 *, glm::mat4 *, camera *);
		void draw(Line line, mat4 *, mat4 *P, mat4 *V, camera *mycam);
		static void handleTexture(char *, GLuint *, int min_mode = GL_LINEAR);
		static void handleTexture(unsigned char*, GLuint*, int, int, int min_mode = GL_LINEAR);
		static void handleBufferText(GLuint*, GLuint, int, int);
	private:
		std::shared_ptr<Program> prog;
		GLuint *TEXTURE_IDS = NULL;
		GLuint VAID, POS_BUF, TEX_BUF, IDX_BUF;
		int numTexts;
		float time = 0;
};

#endif
