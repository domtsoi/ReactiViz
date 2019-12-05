#ifndef __MY_BODY__
#define __MY_BODY__

#include "../../necessary_includes.h"
#include "../Helpers/helpers.h"
#include "../Billboard/Billboard.h"
#include "../../Shape.h"
#include "../../MyMeshBuilder.h"
#include <Kinect.h>
#include "../Frames/Frames.h"

#define NUM_JOINTS 25

class Body
{
	public:
		Body(IBody *, ICoordinateMapper*); 
		static shared_ptr<vector<Body>> get_bodies(IBody**, int, shared_ptr<Frames>, ICoordinateMapper*);
		float compare(Body*);
		void update(IBody*, ICoordinateMapper*);
		void update(Body*);
		vec4 joints[NUM_JOINTS];
	private:
		Body();
};
#endif
