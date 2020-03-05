#include "Kinect.h"

using namespace glm;
using namespace std;

#define MAX_KINECT_BODIES 6
#define DEPTH_FRAME 0
#define COLOR_FRAME 1
#define MIN_COMPARISON 1.0 // TODO: optimize this value
#define NUM_BODIES 6
#define MAX_KINECT_DEPTH 8000.f
#define LIFETIME 10

Kinect::Kinect(int num_bodies, int width, int height)
{
	// NOTE: THIS CODE IS UNSAFE ON PURPOSE : MEANT TO CRASH IF KINECT CAN'T BE SETUP
	this->num_bodies = num_bodies;
	this->width = width;
	this->height = height;
}

void Kinect::init()
{
	// init kinect
	GetDefaultKinectSensor(&kinect);
	kinect->Open();

	// init color frame reader
	IColorFrameSource* color_framesource = NULL;
	kinect->get_ColorFrameSource(&color_framesource);
	color_framesource->OpenReader(&color_reader);
	IFrameDescription* color_descr = NULL;
	color_framesource->get_FrameDescription(&color_descr);
	color_descr->get_Width(&c_width);
	color_descr->get_Height(&c_height);
	color_framesource->Release();

	// init depth frame reader
	IDepthFrameSource* depth_framesource = NULL;
	kinect->get_DepthFrameSource(&depth_framesource);
	depth_framesource->OpenReader(&depth_reader);
	IFrameDescription* depth_descr = NULL;
	depth_framesource->get_FrameDescription(&depth_descr);
	depth_descr->get_Width(&d_width);
	depth_descr->get_Height(&d_height);
	depth_framesource->Release();

	// init body frame reader
	IBodyFrameSource* body_framesource = NULL;
	kinect->get_BodyFrameSource(&body_framesource);
	body_framesource->OpenReader(&body_reader);
	body_framesource->Release();

	// init frame buffers
	uint c_size = c_width * c_height * 4;
	color_buffer = new BYTE[c_size];
	uint d_size = d_width * d_height * 4;
	depth_buffer = new BYTE[d_size];

	// TODO: SET BILLBOARD //
	frames = make_shared<Frames>(NUM_FRAMES);
	board = make_shared<Billboard>();
}

void Kinect::get_depth()
{
	IDepthFrame* depth_frame = NULL;
	static bool inited = false;
	depth_reader->AcquireLatestFrame(&depth_frame);
	if (depth_frame)
	{
		inited = true;
		uint size;
		unsigned short* buf;
		depth_frame->AccessUnderlyingBuffer(&size, &buf);
		for (int i = 0; i < d_width * d_height; ++i)
		{
			// TODO: change logic for better accuracy
			int depth = (int)((((float)buf[i]) / MAX_KINECT_DEPTH) * 256.f);
			for (int j = 0; j < 3; ++j) depth_buffer[(4 * i) + j] = depth; // buf[i]; // % 256;
			depth_buffer[(4 * i) + 3] = 0xff; // alpha = 1
		}
		depth_frame->Release();
	}

	if (inited) frames->set_text_info(DEPTH_FRAME, depth_buffer, d_width, d_height);
}

void Kinect::get_color()
{
	IColorFrame* color_frame = NULL;
	static bool inited = false;
	color_reader->AcquireLatestFrame(&color_frame);
	if (color_frame)
	{
		inited = true;
		color_frame->CopyConvertedFrameDataToArray(c_width * c_height * 4, color_buffer, ColorImageFormat_Bgra);
		color_frame->Release();
	}	
	if (inited) frames->set_text_info(COLOR_FRAME, color_buffer, c_width, c_height);;
}


shared_ptr<vector<Body>> Kinect::get_new_bodies()
{
	IBodyFrame* body_frame = NULL;
	body_reader->AcquireLatestFrame(&body_frame);
	
	if (!body_frame) return make_shared<vector<Body>>();

	IBody* i_bodies[NUM_BODIES] = { 0 };
	body_frame->GetAndRefreshBodyData(num_bodies, i_bodies);
	static ICoordinateMapper* icm = NULL;
	if (!icm) kinect->get_CoordinateMapper(&icm);
	shared_ptr<vector<Body>> recent_bodies = Body::get_bodies(i_bodies, num_bodies, frames, icm);

	body_frame->Release();
	return recent_bodies;
}


void Kinect::update_bodies()
{
	shared_ptr<vector<Body>> new_bodies = get_new_bodies();

	// find bodies that match current bodies
	// TODO: consider having this code be in compute shader
	for (int i = 0; i < bodies->size(); ++i)
	{
		bool updated = false;
		for (int j = 0; j < new_bodies->size(); ++j)
		{
			if (bodies->at(i).compare(&new_bodies->at(j)) < MIN_COMPARISON)
			{
				updated = true;
				bodies->at(i) = new_bodies->at(j);
				if (body_timestamp->at(i) < 0) ++body_timestamp->at(i);
				else if (body_timestamp->at(i) > 0) --body_timestamp->at(i);
				new_bodies->erase(new_bodies->begin() + j); // remove "old" bodies from new bodies
			}
		}

		if (!updated) ++body_timestamp->at(i);
	}

	// remove old bodies
	shared_ptr<vector<Body>> temp_bodies = make_shared<vector<Body>>();
	shared_ptr<vector<int>> temp_timestamp = make_shared<vector<int>>();
	for (int i = 0; i < bodies->size(); ++i)
	{
		if (body_timestamp->at(i) <= LIFETIME)
		{
			temp_bodies->push_back(bodies->at(i));
			temp_timestamp->push_back(body_timestamp->at(i));
		}
	}
	bodies.reset();
	body_timestamp.reset();
	bodies = temp_bodies;
	body_timestamp = temp_timestamp;

	for (int i = 0; i < (num_bodies - bodies->size()) && i < new_bodies->size(); ++i)
	{
		bodies->push_back(new_bodies->at(i));
		body_timestamp->push_back(0); // set initial timestamp to 0
	}
	new_bodies.reset();
}

void Kinect::draw_bodies()
{
	board->draw(frames, bodies, body_timestamp);
}

void Kinect::draw_bodies(float music_influence, GLuint TVtex, float kinect_depth)
{
	board->draw(frames, bodies, body_timestamp, music_influence, TVtex, kinect_depth);
}