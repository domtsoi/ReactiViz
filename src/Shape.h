

#pragma once

#ifndef LAB471_SHAPE_H_INCLUDED
#define LAB471_SHAPE_H_INCLUDED

#include <string>
#include <vector>
#include <memory>






class Program;

class Shape
	{

	public:
		//stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp)
		void loadMesh(const std::string &meshName, std::string *mtlName = NULL, unsigned char *(loadimage)(char const *, int *, int *, int *, int) = NULL);
		void init();
		void init(int subobj, bool instance_subobj = false, int pro_instancepos_location = 0, std::vector<float> * vec4_positionbuf = NULL, int instance_datatype_size_ = 0);
		void resize();
		void resize_subobj();
		void draw(const std::shared_ptr<Program> prog, bool use_extern_texures) const;
		void draw(int subobj,const std::shared_ptr<Program> prog, bool use_extern_texures) const;
		unsigned int *textureIDs = NULL;
		int return_subsize() { return obj_count; }

	private:
		
		bool instance_subobject = false;
		
		int obj_count = 0;
		std::vector<unsigned int> *eleBuf = NULL;
		std::vector<float> *posBuf = NULL;
		std::vector<float> *norBuf = NULL;
		std::vector<float> *texBuf = NULL;
		unsigned int *materialIDs = NULL;

		unsigned int* InstBufID = 0;
		unsigned int* instance_size = 0;
		unsigned int instance_datatype_size = 0;
		

		unsigned int *eleBufID = 0;
		unsigned int *posBufID = 0;
		unsigned int *norBufID = 0;
		unsigned int *texBufID = 0;
		unsigned int *vaoID = 0;

	};

#endif // LAB471_SHAPE_H_INCLUDED
