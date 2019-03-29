#pragma once
#include <vector>
class CCL3D
{
public:

	int depth;
	int width;
	int height;
	int currentLabelCount;
	short **label;
	int *eqv;
	std::vector <int> count;
	std::vector <int> fcount;
	short **image;
	short **image_vi;

	
	
	void InitMembers(short ** image, int d, int w, int h);
	void firstpass();
	void firstpassEcon();
	short find(short a);
	
	void secondpass();
	void visualize();

	short ** getImage();
	void CCL3D::Rootunion(short a, short b);
	
	CCL3D();
	~CCL3D();

};

