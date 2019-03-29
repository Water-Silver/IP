#pragma once
#include <vector>
#include "Points.h"
using namespace std;
class Powell
{
public:

	
	short** distanceMap;
	short** floatingImg;
	int intsum;
	int totalsum;
	int floatingdepth;
	int depth;
	int width;
	int height;
	double ** T;
	double PI;
	int centerx;
	int centery;
	int centerz;
	double symMetric;

	// transform matrix that would store the whole process



	std::vector<Points> P;


	//Powell(short **distanceMap, short**floating, std::vector<Points> P);

	//void InitMembers(short ** distanceMap, short ** floating, std::vector<Points> P);

	//void MatMul(int * Transformation[4], int * T[4]);

	//void MatMul(int Transformation[][4], int T[][4]);

	//void MatMul(double(&Transformation)[4][4], double(&T)[4][4]);

	//void MatMul(double ** Transformation, double **T);

	Powell(short ** distanceMap, short ** floating, std::vector<Points> P, int depth1, int depth2, int x, int y, int z);

	void MatMul(double ** Transformation);


	//void MatMul(double Transformation[][4], double **T);

	std::vector<Points> TranslateX(vector<Points> P, int winSize);

	
	std::vector<Points> TranslateY(vector<Points> P,  int winSize);

	std::vector<Points> TranslateZ(vector<Points> P,  int winSize);

	std::vector<Points> RotationX(vector<Points> P, int winSize);

	std::vector<Points> RotationY(vector<Points> P,int winSize);

	std::vector<Points> RotationZ(vector<Points> P, int winSize);

	int PowIter(short ** distanceMap, short ** floatingImg);

	int PowIterNew(short ** distanceMap, short ** floatingImg);

	Powell();
	~Powell();
};

