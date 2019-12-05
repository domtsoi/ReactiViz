#include "Body.h"

#define D_WIDTH 512.f
#define D_HEIGHT 424.f

Body::Body(IBody* body, ICoordinateMapper* icm)
{
	update(body, icm);
}

float Body::compare(Body* body)
{
	float diff = 0;
	for (int i = 0; i < NUM_JOINTS; ++i)
	{
		diff += length((joints[i] - body->joints[i])); // .length();
	}
	return diff;
}

void Body::update(IBody* body, ICoordinateMapper *icm)
{
	Joint body_joints[NUM_JOINTS];
	body->GetJoints(NUM_JOINTS, body_joints);

	for (int i = 0; i < NUM_JOINTS; ++i)
	{
		DepthSpacePoint dsp;
		icm->MapCameraPointsToDepthSpace(1, &body_joints[i].Position, 1, &dsp);
		joints[i] = vec4(dsp.X / D_WIDTH, dsp.Y / D_HEIGHT, 0, 0);
	}
}

void Body::update(Body* body)
{
	for (int i = 0; i < NUM_JOINTS; ++i) joints[i] = body->joints[i];
}

shared_ptr<vector<Body>> Body::get_bodies(IBody** i_bodies, int num_bodies, shared_ptr<Frames> frames, ICoordinateMapper* icm)
{
	shared_ptr<vector<Body>> bodies = make_shared<vector<Body>>();
	// set up bodies
	for (int i = 0; i < num_bodies; ++i)
	{
		if (i_bodies[i])
		{
			BOOLEAN is_tracked;
			i_bodies[i]->get_IsTracked(&is_tracked);
			if (is_tracked) 
				bodies->push_back(Body(i_bodies[i], icm));
		}
	}
	// use compute shader to get depth of the bodies
	frames->compute_body_depth(bodies);
	return bodies;
}