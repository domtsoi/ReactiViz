#pragma once


#ifndef LAB471_CAMERA_H_INCLUDED
#define LAB471_CAMERA_H_INCLUDED

#include <stack>
#include <memory>

#include "glm/glm.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
using namespace glm;
using namespace std;

enum rendermodes { MODE_CITYFWD, MODE_CITYSTATIC, MODE_LANDFWD, MODE_LANDSTATIC,MODE_UNKNOWN, MODE_TUNNEL};


class camera
{
private:
	float speed;
public:
	bool toggle_auto;
	glm::vec3 pos;
	glm::vec3 rot;
	glm::mat4 V;
	glm::mat4 R, Rtoggle_y, Rtoggle_x, Ttoggle;
	vec3 oldrot;
	int w, a, s, d;
	bool toggleview;
	float old_going_forward;
	void toggle() { toggleview = !toggleview; }
	float delay(float old, float actual, float mul)
		{
	//	if (actual > old) return actual;
		float fold = (float)old;
		float factual = (float)actual;
		float fres = fold - (fold - factual) * mul;
		return (float)fres;
		}
	float get_speed()
		{
		if (toggleview)return -speed;
		return speed;
		}

	camera()
		{
		reset(MODE_UNKNOWN);
		}
	void reset(rendermodes rendermode)
	{
		
		old_going_forward = 0.0;
		speed = 0;
		toggleview = false;
		toggle_auto = false;
		w = a = s = d = 0;
		pos = rot = glm::vec3(0, 0, 0);
		oldrot = vec3(0);
		V = R = glm::mat4(1);
		Rtoggle_y = glm::rotate(glm::mat4(1), (float)3.1415926, glm::vec3(0, 1, 0));
		Rtoggle_x = glm::rotate(glm::mat4(1), (float)-0.3, glm::vec3(1, 0, 0));
		Ttoggle = translate(glm::mat4(1), glm::vec3(0.0, 0, -0.04));

		if (rendermode == MODE_LANDFWD || rendermode == MODE_LANDSTATIC)
		{
			rot.x = 0.1;
			oldrot.x = 0.1;
			
		}
		if (rendermode == MODE_LANDFWD) toggle_auto = true;
		if (rendermode == MODE_CITYFWD) toggle_auto = true;
		if (rendermode == MODE_TUNNEL) toggle_auto = true;
		}

	vec3 rot_diff() {
		vec3 diff = rot - oldrot;
		//cout << diff.y << " " << rot.y << " " << oldrot.y<< endl;
		return diff; 
		}
	glm::mat4 process(float frametime, rendermodes rendermode)
	{
		float speed = 9;
		switch (rendermode)
			{
			default:
			case MODE_CITYFWD:
			case MODE_CITYSTATIC:
			speed = 9;
			break;
			case MODE_LANDFWD:
			case MODE_LANDSTATIC:
			speed = 30;
			break;
			case MODE_TUNNEL:
			speed = 20;
			break;
			}
		
		//frametime = 0.01;
		float going_forward = 0.0;
		static vec3 actionrot;
		if (toggle_auto)
			going_forward -= speed * frametime;
		if (w == 1)
			going_forward -= speed * frametime*20.;
		if (s == 1)
			going_forward += speed * frametime;
		if (a == 1)
			rot.y += 1.2* frametime;
		if (d == 1)
			rot.y -= 1.2* frametime;
		
		float mul = 0.07;
		
		old_going_forward = delay(old_going_forward, going_forward, mul);

		speed = -old_going_forward;

		mul = 0.1;
		
		actionrot.x = delay(oldrot.x, rot.x, mul);
		actionrot.y = delay(oldrot.y, rot.y, mul);
		actionrot.z = delay(oldrot.z, rot.z, 0.005);
		//actionrot = rot;
		oldrot = actionrot;
		R = glm::rotate(glm::mat4(1), actionrot.y, glm::vec3(0, 1, 0));
		mat4 Rz = glm::rotate(glm::mat4(1), actionrot.z, glm::vec3(0, 0, 1));
		mat4 Rx = glm::rotate(glm::mat4(1), actionrot.x, glm::vec3(1, 0, 0));

		R = Rz * R;

		glm::vec4 offset = glm::vec4(0);
		if (toggleview)
			{
			float y_offset = actionrot.z * 0.0009;
			offset = glm::vec4(0, y_offset, -0.0355, 1);
			offset = R * offset;
			}
	
		glm::vec4 rpos = glm::vec4(0, 0, old_going_forward, 1);

		rpos = R *rpos;
		pos.x += rpos.x;
		pos.z += rpos.z;
		glm::vec4 lookto = glm::vec4(0, 0, -10.001, 0);
		lookto =(R *  Rx) * lookto;

		if(toggleview)
			lookto = (Rtoggle_x*Rtoggle_y) * lookto;
		glm::mat4 T = glm::translate(glm::mat4(1), glm::vec3(pos.x, pos.y, pos.z) + glm::vec3(offset));

		glm::mat4 Vc = glm::lookAt(pos+ glm::vec3(offset), pos + glm::vec3(offset) + glm::vec3(lookto.x, lookto.y, lookto.z) , glm::vec3(0.0f, 1.0f, 0.0f));

		
		//invV_laser = glm::lookAt(glm::vec3(0), glm::vec3(lookto.x, lookto.y, lookto.z), glm::vec3(0.0f, 1.0f, 0.0f));
		//invV_laser = inverse(invV_laser);
		
		Vc = Rz * Vc;
		V =  Vc;
		return V;
		return R*T;
	}

};








#endif // LAB471_CAMERA_H_INCLUDED