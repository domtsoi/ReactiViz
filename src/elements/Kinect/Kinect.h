#ifndef __MY_KINECT__H__
#define __MY_KINECT__H__

#define NUM_FRAMES 6

#include "../../necessary_includes.h"
#include "../Helpers/helpers.h"
#include "../Billboard/Billboard.h"
#include "../../Shape.h"
#include "../../MyMeshBuilder.h"
#include "../Body/Body.h"
#include "../Frames/Frames.h"
#include <Kinect.h>

#define NUM_FRAMES 2

class Kinect
{
public:
	Kinect(int, int, int);
	void get_depth();
	void get_color();
	void update_bodies();
	void draw_bodies();
	void draw_bodies(float,GLuint,float);
	void init();
private:
	Kinect();
	// Kinect related resources
	IKinectSensor *kinect = NULL;
	IColorFrameReader *color_reader = NULL;
	int c_width, c_height;
	IDepthFrameReader *depth_reader = NULL;
	int d_width, d_height;
	IBodyFrameReader *body_reader = NULL;
	// screen related info
	int width, height;
	// Body specific related resources
	int num_bodies;
	shared_ptr<vector<Body>> bodies = make_shared<vector<Body>>();
	shared_ptr<vector<int>> body_timestamp = make_shared<vector<int>>();
	shared_ptr<Frames> frames = NULL; 
	shared_ptr<Billboard> board = NULL;
	shared_ptr<vector<Body>> get_new_bodies();
	// helper stuff
	BYTE* depth_buffer = NULL;
	BYTE* color_buffer = NULL;
};


#endif