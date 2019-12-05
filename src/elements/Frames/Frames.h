#ifndef __FRAMES__H__
#define __FRAMES__H__

#include "../../necessary_includes.h"
#include "../Helpers/helpers.h"
#include "../../Shape.h"
#include "../../MyMeshBuilder.h"
#include <Kinect.h>

class Body;

class Frames
{
public:
	Frames(int);
	void set_text_info(int, BYTE*, int, int);
	void compute_body_depth(shared_ptr<vector<Body>>);
	GLuint get_text_id(int id) { return text_ids[id]; };
	GLuint get_render_text_id(int rtid) { return render_objects[rtid].tid; };
	void bind_render_object(int ro) { render_objects[ro].bind(); }
	static void unbind_render_objects();
	shared_ptr<Program> prog;
private:
	Frames();
	GLuint compute_prog;
	uint num_texts;
	GLuint* text_ids;
	GLuint ssbo_bodies;
	void init_compute_prog();
	class RenderObject
	{
	public:
		GLuint fbo, rbo, tid;
		int width = 1920, height = 1080;
		RenderObject();
		void bind();
		static void unbind();
	};
	RenderObject* render_objects;
};

#endif