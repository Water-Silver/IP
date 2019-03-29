#include "Powell.h"

#include <stdio.h>
#include "CCL.h"
#include <vector>
#include <cstring>
#include <numeric>
#include <omp.h>
#include <algorithm>
#include "Points.h"
#define max 255
//#define PI 1.57079 // PI /2


//point 들의 위치를 어떻게 저장해주지. 클래스 만들면 메모리 초과할거 같은데.

using namespace std;



template <typename T>
vector<size_t> sort_indexes(const vector<T> &v) {

	// initialize original index locations
	vector<size_t> idx(v.size());
	iota(idx.begin(), idx.end(), 0);

	// sort indexes based on comparing values in v
	sort(idx.begin(), idx.end(),
		[&v](size_t i1, size_t i2) {return v[i1] > v[i2]; });

	return idx;
}

Powell::Powell(short **distanceMap, short**floating, std::vector<Points> P, int depth1, int depth2, int x, int y, int z) :
	distanceMap(distanceMap),
	floatingImg(floating),
	P(P),
	depth(depth2),
	floatingdepth(depth1),
	width(512),
	height(512),
	centerx(x),
	centery(y),
	centerz(z),
	PI(1.57079), // PI/2
	symMetric(0)
{
	T = new double *[4];
	for (int i = 0; i < 4; i++) {
		T[i] = new double[4];
	}

	
	for (int i = 0; i < 4; i++) {

		for (int j = 0; j < 4; j++) {
			T[i][j] = 0.0;
			//printf_s("%lf\n", T[i][j]);
		}
	}
	

	T[0][0] = T[1][1] = T[2][2] = T[3][3] = 1; //identity matrix
	printf_s("init!!\n");
	
}

//void Powell::MatMul(double ** Transformation, double **T) { // works only for 4*4 matrix
void Powell::MatMul(double ** Transformation) { // works only for 4*4 matrix

	
	double output[4][4];

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			output[i][j] = 0;
			for (int k = 0; k < 4; k++) {
				// matrix multiplication 어떻게 하더라.
				output[i][j] += Transformation[i][k] * T[k][j];
				
			}
		}
	}

	
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			printf_s("%lf ", output[i][j]);
			T[i][j] = output[i][j];
		}
		printf_s("\n");
	}
	printf_s("\n");
	
}

std::vector<Points> Powell::TranslateX(vector<Points> P, int winSize) {
	

	vector<int> localsum(winSize);
	vector<int> symCount(winSize);
	
	vector <vector<Points>  > Xtransform(winSize, vector<Points>(P.size())); // points  를 담은   vector 들을 winSize 만큼 가지려면?? 

	
	for (int i = 0; i < winSize; i++) { // initialize the transformed vector sets.
		for (int j = 0; j < P.size(); j++) {

			Xtransform[i].at(j).w = P.at(j).w - winSize/2 + i; 
			Xtransform[i].at(j).h = P.at(j).h;
			Xtransform[i].at(j).d = P.at(j).d;
		}

	}

	printf_s("P.w = %d P.h = %d P.d = %d\n", P.at(3).w, P.at(3).h, P.at(3).d);
	

	for (int i = 0; i < winSize; i++) {
		for (int j = 0; j < Xtransform[i].size(); j++) {
			Points a = Xtransform[i].at(j);
			if (a.d + centerz < 0 || a.d + centerz >= depth || a.w + centerx < 0 || a.w + centerx >= width || a.h + centery < 0 || a.h + centery >= height) {
				localsum.at(i) += 155;
			}
			else {
				localsum.at(i) += distanceMap[a.d + centerz][a.w + centerx + (a.h + centery) * 512];
				symCount.at(i)++;
			}
		}
	}

	vector<short> index(winSize); int k = 0;
	for (auto i : sort_indexes(localsum)) { // 들어 있는 값이 큰 순서대로 인덱스를 반환하는 함수
		index.at(k++) = i;
	}

	int idx = index.back(); // get the index with minimum value
	symMetric = symCount.at(idx);// (double)localsum.at(idx) / symCount.at(idx);


	printf_s("P.w = %d P.h = %d P.d = %d\n", Xtransform[idx].at(3).w, Xtransform[idx].at(3).h, Xtransform[idx].at(3).d);
	//Translate matrix  만들어야.
	



	double **Tx = new double *[4];
	for (int i = 0; i < 4; i++) {
		Tx[i] = new double[4];
	}
	
	//double ** Tx [4][4];
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			Tx[i][j] = 0;
			if (i == j) Tx[i][j] = 1;
			
		}
	}

	Tx[0][3] = -winSize/2 + idx;


	MatMul(Tx); 

	this->totalsum = localsum.at(idx);
	printf_s("localsum = %d\n", localsum.at(idx));
	printf_s("symMetric = %lf\n", symMetric);

	return Xtransform[idx]; // return the transformed set of points with local minimum sum  // 이  matrix 저장도 해야할거 같음... 나중에 전체 이미지? 보내는 transform 수행할 때 필요.
	// 여기 질문.

}

std::vector<Points> Powell::TranslateY(vector<Points> P, int winSize) {
	// 여기 벡터 받아서 벡터 넘기는거로 바꿀거임.

	vector<int> localsum(winSize);
	vector<int> symCount(winSize);

	vector <vector<Points>  > Ytransform(winSize, vector<Points>(P.size())); // points  를 담은   vector 들을 winSize 만큼 가지려면?? 

	for (int i = 0; i < winSize; i++) { // initialize the transformed vector sets.
		for (int j = 0; j < P.size(); j++) {

			Ytransform[i].at(j).w = P.at(j).w;
			Ytransform[i].at(j).h = P.at(j).h - winSize / 2 + i;
			Ytransform[i].at(j).d = P.at(j).d;
		}

	}
	printf_s("P.w = %d P.h = %d P.d = %d\n", P.at(3).w, P.at(3).h, P.at(3).d);

	for (int i = 0; i < winSize; i++) {
		for (int j = 0; j < Ytransform[i].size(); j++) {
			Points a = Ytransform[i].at(j);
			if (a.d + centerz < 0 || a.d + centerz >= depth || a.w + centerx < 0 || a.w + centerx >= width || a.h + centery < 0 || a.h + centery >= height) {
				localsum.at(i) += 155;
			}
			else {
				localsum.at(i) += distanceMap[a.d + centerz][a.w + centerx + (a.h + centery) * 512];
				symCount.at(i)++;

			}
		}
	}

	vector<short> index(winSize); int k = 0;
	for (auto i : sort_indexes(localsum)) { // 들어 있는 값이 큰 순서대로 인덱스를 반환하는 함수
		index.at(k++) = i;
	}

	int idx = index.back(); // get the index with minimum value

	symMetric = symCount.at(idx);// (double) localsum.at(idx) / symCount.at(idx);
	printf_s("P.w = %d P.h = %d P.d = %d\n", Ytransform[idx].at(3).w, Ytransform[idx].at(3).h, Ytransform[idx].at(3).d);

	double **Ty = new double *[4];
	for (int i = 0; i < 4; i++) {
		Ty[i] = new double[4];
	}
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			Ty[i][j] = 0;
			if (i == j) Ty[i][j] = 1;

		}
	}

	Ty[1][3] = -winSize / 2 + idx;



	MatMul(Ty);

	this->totalsum = localsum.at(idx);
	printf_s("localsum = %d\n", localsum.at(idx));
	printf_s("symMetric = %lf\n", symMetric);
	return Ytransform[idx]; // return the transformed set of points with local minimum sum  // 이  matrix 저장도 해야할거 같음... 나중에 전체 이미지? 보내는 transform 수행할 때 필요.
	// 여기 질문.

}




std::vector<Points> Powell::TranslateZ(vector<Points> P, int winSize) {
	// 여기 벡터 받아서 벡터 넘기는거로 바꿀거임.

	vector<int> localsum(winSize);
	vector<int> symCount(winSize);
	vector <vector<Points>  > Ztransform(winSize, vector<Points>(P.size())); // points  를 담은   vector 들을 winSize 만큼 가지려면?? 

	for (int i = 0; i < winSize; i++) { // initialize the transformed vector sets.
		for (int j = 0; j < P.size(); j++) {

			Ztransform[i].at(j).w = P.at(j).w;
			Ztransform[i].at(j).h = P.at(j).h;
			Ztransform[i].at(j).d = P.at(j).d - winSize / 2 + i;
		}

	}
	
	printf_s("P.w = %d P.h = %d P.d = %d\n", P.at(3).w, P.at(3).h, P.at(3).d);


	for (int i = 0; i < winSize; i++) {
		for (int j = 0; j < Ztransform[i].size(); j++) {
			Points a = Ztransform[i].at(j);
			if (a.d + centerz < 0 || a.d + centerz >= depth || a.w + centerx < 0 || a.w + centerx >= width || a.h + centery < 0 || a.h + centery >= height) {
				localsum.at(i) += 155;
			}
			else {
				localsum.at(i) += distanceMap[a.d + centerz][a.w + centerx + (a.h + centery) * 512];
				symCount.at(i)++;
			}
		}
	}

	vector<short> index(winSize); int k = 0;
	for (auto i : sort_indexes(localsum)) { // 들어 있는 값이 큰 순서대로 인덱스를 반환하는 함수
		index.at(k++) = i;
	}

	int idx = index.back(); // get the index with minimum value
	symMetric = symCount.at(idx);// (double) localsum.at(idx) / symCount.at(idx);
	printf_s("P.w = %d P.h = %d P.d = %d\n", Ztransform[idx].at(3).w, Ztransform[idx].at(3).h, Ztransform[idx].at(3).d);

	double **Tz = new double *[4];
	for (int i = 0; i < 4; i++) {
		Tz[i] = new double[4];
	}

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (i == j) Tz[i][j] = 1;
			else Tz[i][j] = 0;
			

		}
	}

	Tz[2][3] = -winSize/2+idx;



	MatMul(Tz); 

	this->totalsum = localsum.at(idx);
	printf_s("localsum = %d\n", localsum.at(idx));
	printf_s("symMetric = %lf\n", symMetric);

	return Ztransform[idx]; // return the transformed set of points with local minimum sum  // 이  matrix 저장도 해야할거 같음... 나중에 전체 이미지? 보내는 transform 수행할 때 필

}





std::vector<Points> Powell::RotationX(vector<Points> P, int winSize) {
	

	vector<int> localsum(winSize);
	vector<int> symCount(winSize);

	vector <vector<Points>	> XRotate(winSize, vector<Points>(P.size()));

	printf("in RotationX\n");
	for (int i = 0; i < winSize; i++) { // initialize the transformed vector sets.
		for (int j = 0; j < P.size(); j++) {

			XRotate[i].at(j).w = P.at(j).w;
			XRotate[i].at(j).h = (int)(floor(P.at(j).h * cos((-winSize /2 + i)*PI/180)- P.at(j).d * sin((-winSize / 2 + i)*PI / 180) + 0.5)); // 반올림 후 int 형으로 형변환
			XRotate[i].at(j).d = (int)(floor(P.at(j).h*sin((-winSize / 2 + i)*PI / 180)+P.at(j).d * cos((-winSize / 2 + i)*PI / 180) + 0.5));
			//if(i==winSize-1) printf("%d %d\n", XRotate[i].at(j).h, XRotate[i].at(j).d);
			
			

		}
	}

	printf_s("P.w = %d P.h = %d P.d = %d\n", P.at(3).w, P.at(3).h, P.at(3).d);
	
	
	for (int i = 0; i < winSize; i++) {
		
		for (int j = 0; j < XRotate[i].size(); j++) {
			Points a = XRotate[i].at(j);

			if (a.d + centerz < 0 || a.d + centerz >= depth || a.w + centerx < 0 || a.w + centerx >= width || a.h + centery < 0 || a.h + centery >= height) {
				localsum.at(i) += 155;
			}
			else {
				localsum.at(i) += distanceMap[a.d + centerz][a.w + centerx + (a.h + centery) * 512];
				symCount.at(i)++;
				
			}
		}
	}

	vector<short> index(winSize); int k = 0;
	for (auto i : sort_indexes(localsum)) { // 들어 있는 값이 큰 순서대로 인덱스를 반환하는 함수
		index.at(k++) = i;
	}

	int idx = index.back(); // get the index with minimum value
	symMetric = symCount.at(idx);// (double) localsum.at(idx) / symCount.at(idx);

	printf_s("P.w = %d P.h = %d P.d = %d\n", XRotate[idx].at(3).w, XRotate[idx].at(3).h, XRotate[idx].at(3).d);
	printf_s("%d\n", idx);

	double **Rx = new double *[4];
	for (int i = 0; i < 4; i++) {
		Rx[i] = new double[4];
	}
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (i == j) Rx[i][j] = 1;
			else Rx[i][j] = 0;
		}
	}
	//printf_s("idx = %d\n", idx);
	Rx[1][1] = cos((-winSize/2 + idx)*PI/180);
	Rx[1][2] = -sin((-winSize/2 + idx)*PI/180);
	Rx[2][1] = sin((-winSize/2 + idx)*PI/180);
	Rx[2][2] = cos((-winSize/2 +  idx)*PI/180);



	MatMul(Rx); 

	this->totalsum = localsum.at(idx);
	printf_s("localsum = %d\n", localsum.at(idx));
	printf_s("symMetric = %lf\n", symMetric);
	return XRotate[idx]; // return the transformed set of points with local minimum sum  // 이  matrix 저장도 해야할거 같음... 나중에 전체 이미지? 보내는 transform 수행할 때 필요.
	
	
}


std::vector<Points> Powell::RotationY(vector<Points> P, int winSize) {
	
	vector<int> localsum(winSize);
	vector<int> symCount(winSize);
	vector <vector<Points>	> YRotate(winSize, vector<Points>(P.size()));
	printf("in RotationY\n");

	for (int i = 0; i < winSize; i++) { // initialize the transformed vector sets.
		for (int j = 0; j < P.size(); j++) {

			YRotate[i].at(j).w = (int) (floor(P.at(j).w*cos((-winSize / 2 +i)*PI / 180) + P.at(j).d * sin((-winSize / 2 + i)*PI / 180) + 0.5));
			YRotate[i].at(j).h = P.at(j).h;
			YRotate[i].at(j).d = (int) (floor(-P.at(j).w * sin((-winSize / 2 + i)*PI / 180) +P.at(j).d * cos((-winSize / 2 + i)*PI / 180) + 0.5));
		}

	}

	printf_s("P.w = %d P.h = %d P.d = %d\n", P.at(3).w, P.at(3).h, P.at(3).d);

	for (int i = 0; i < winSize; i++) {
		for (int j = 0; j < YRotate[i].size(); j++) {
			Points a = YRotate[i].at(j);
			if (a.d + centerz < 0 || a.d + centerz >= depth || a.w + centerx < 0 || a.w + centerx >= width || a.h + centery < 0 || a.h + centery >= height) {
				localsum.at(i) += 155;
			}
			else {
				localsum.at(i) += distanceMap[a.d + centerz][a.w + centerx + (a.h + centery) * 512];
				symCount.at(i)++;
				
			}
		}
	}

	vector<short> index(winSize); int k = 0;
	for (auto i : sort_indexes(localsum)) { // 들어 있는 값이 큰 순서대로 인덱스를 반환하는 함수
		index.at(k++) = i;
	}

	int idx = index.back(); // get the index with minimum value
	symMetric = symCount.at(idx);// (double) localsum.at(idx) / symCount.at(idx);

	printf_s("P.w = %d P.h = %d P.d = %d\n", YRotate[idx].at(3).w, YRotate[idx].at(3).h, YRotate[idx].at(3).d);
	printf_s("%d\n", idx);
	double **Ry = new double *[4];
	for (int i = 0; i < 4; i++) {
		Ry[i] = new double[4];
	}
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (i == j) Ry[i][j] = 1;
			else Ry[i][j] = 0;
			

		}
	}

	Ry[0][0] = cos((-winSize / 2 +  idx)*PI/180);
	Ry[0][2] = sin((-winSize / 2 +  idx)*PI/180);
	Ry[2][0] = -sin((-winSize / 2 + idx)*PI/180);
	Ry[2][2] = cos((-winSize / 2 +  idx)*PI/180);

	MatMul(Ry);

	this->totalsum = localsum.at(idx);
	printf_s("localsum = %d\n", localsum.at(idx));
	printf_s("symMetric = %lf\n", symMetric);
	return YRotate[idx];

}

std::vector<Points> Powell::RotationZ(vector<Points> P, int winSize) {
	
	vector<int> localsum(winSize);
	vector<int> symCount(winSize);
	vector <vector<Points>	> ZRotate(winSize, vector<Points>(P.size()));

	printf("in RotationZ\n");

	for (int i = 0; i < winSize; i++) { // initialize the transformed vector sets.
		for (int j = 0; j < P.size(); j++) {

			ZRotate[i].at(j).w = (int)(floor(P.at(j).w*cos((-winSize/2 + i)*PI/180) - P.at(j).h * sin((-winSize/2 + i)*PI/180) +0.5));
			ZRotate[i].at(j).h = (int) (floor(P.at(j).w * sin((-winSize / 2 +i)*PI/180) + P.at(j).h * cos((-winSize/2 + i)*PI/180) + 0.5));
			ZRotate[i].at(j).d = P.at(j).d;
		}

	}
	
	printf_s("P.w = %d P.h = %d P.d = %d\n", P.at(3).w, P.at(3).h, P.at(3).d);
	
	for (int i = 0; i < winSize; i++) {
		for (int j = 0; j < ZRotate[i].size(); j++) {
			Points a = ZRotate[i].at(j);
			if (a.d + centerz < 0 || a.d + centerz >= depth || a.w + centerx < 0 || a.w + centerx >= width || a.h + centery < 0 || a.h + centery >= height) {
				localsum.at(i) += 155;
			}
			else {
				localsum.at(i) += distanceMap[a.d + centerz][a.w + centerx + (a.h + centery) * 512];
				symCount.at(i)++;
				
			}
		}
	}

	vector<short> index(winSize); int k = 0;
	for (auto i : sort_indexes(localsum)) { // 들어 있는 값이 큰 순서대로 인덱스를 반환하는 함수
		index.at(k++) = i;
	}

	int idx = index.back(); // get the index with minimum value
	symMetric = symCount.at(idx);// (double) localsum.at(idx) / symCount.at(idx);
	printf_s("P.w = %d P.h = %d P.d = %d\n", ZRotate[idx].at(3).w, ZRotate[idx].at(3).h, ZRotate[idx].at(3).d);
	printf_s("%d\n", idx);
	double **Rz = new double *[4];
	for (int i = 0; i < 4; i++) {
		Rz[i] = new double[4];
	}

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (i == j) Rz[i][j] = 1;
			else Rz[i][j] = 0;
			

		}
	}

	Rz[0][0] = cos((-winSize/2 +  idx)*PI / 180);
	Rz[0][1] = -sin((-winSize/2 + idx)*PI / 180);
	Rz[1][0] = sin((-winSize/2 +  idx)*PI / 180);
	Rz[1][1] = cos((-winSize/2 +  idx)*PI / 180);

	MatMul(Rz);
	printf_s("localsum = %d\n", localsum.at(idx));
	printf_s("symMetric = %lf\n", symMetric);

	this->totalsum = localsum.at(idx);
	

	//printf_s("rotate around %d", -10 + 5 * idx);

	return ZRotate[idx];
}




int Powell::PowIter(short **distanceMap, short** floatingImg ) { // img1 is the floating img
	int sum=0;
	int depth = 58;
	int width = 512;
	int height = 512;
	int rocount = 0;

	int winSize = 10;
	int presum = 1;
	int preSym = 0;
	totalsum = P.size();
	printf_s("number of edge points = %d\n", totalsum);
	while (abs(presum - totalsum) > 1) {
		//if (symMetric < preSym) break; // 잠깐 symMetric 에 다른 값 넣어줌. 혹시 범위를 벗어나는 값이 늘어나서 symMetric 이 줄어드는 경우 있을까봐 
		/*
		if (symMetric < 3.5) { // 이전의 변화가 지금의 변화량 보다 크면 각도도 10분의 1 배 해주고, winSize도 줄여줌
			this->PI = this->PI/10.0;
			if(winSize!=0)winSize--;
			printf_s("in case 1\n");
		}
		
		else if (symMetric < 2.5){
			this->PI = this->PI/10.0;
			if (winSize != 0)winSize--;
			printf_s("in case 2\n");
		}
		*/

		this->P = TranslateX(this->P, winSize);
		preSym = symMetric; // 한 번  rotation 할 때마다  refresh // 도중에  symMetric 줄어들면 캐치
	
		this->P = TranslateY(this->P, winSize);
		
		presum = totalsum;
		this->P = TranslateZ(this->P, winSize);
		
		
	
		rocount++;
		printf_s("presum = %d totalsum = %d rotation count = %d\n", presum, totalsum, rocount);
	}
	//printf_s("presum = %d totalsum = %d rotation count = %d\n", presum, totalsum, rocount);

	presum = 1;
	while (abs(presum - totalsum) > 1) {
		if (symMetric < preSym) break; // 잠깐 symMetric 에 다른 값 넣어줌. 혹시 범위를 벗어나는 값이 늘어나서 symMetric 이 줄어드는 경우 있을까봐 
		/*
		if (symMetric < 3.5) { // 이전의 변화가 지금의 변화량 보다 크면 각도도 10분의 1 배 해주고, winSize도 줄여줌
			this->PI = this->PI/10.0;
			if(winSize!=0)winSize--;
			printf_s("in case 1\n");
		}

		else if (symMetric < 2.5){
			this->PI = this->PI/10.0;
			if (winSize != 0)winSize--;
			printf_s("in case 2\n");
		}
		*/
		this->P = RotationY(this->P, winSize);
		this->P = RotationX(this->P, winSize);
		presum = totalsum;
		this->P = RotationZ(this->P, winSize);
		rocount++;

		/*
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				printf_s("%d ", T[i][j]);
			}
			printf_s("\n");
		}
		*/
		//RotationZ 하면 totalsum 이 initialize 됨.
		printf_s("presum = %d totalsum = %d rotation count = %d\n", presum, totalsum, rocount);
	}
	//printf_s("presum = %d totalsum = %d rotation count = %d\n", presum, totalsum, rocount);
	return sum;
}



int Powell::PowIterNew(short **distanceMap, short** floatingImg) { // img1 is the floating img
	int sum = 0;
	int depth = 58;
	int width = 512;
	int height = 512;
	int rocount = 0;
	//여기서 iteration 을 돌아야지.......
	int winSize = 10;
	int presum = 1;
	
	int preSym = 0;
	totalsum = P.size();
	int outerpresum = totalsum;
	printf_s("number of edge points = %d\n", totalsum);

	do{
		outerpresum = totalsum;
		do {
			presum = totalsum;
			this->P = TranslateX(this->P, winSize);
			printf_s("presum = %d totalsum = %d rotation count = %d\n\n", presum, totalsum, rocount);
		} while (abs(presum - totalsum) > 1);
	
		do {
			presum = totalsum;
			this->P = TranslateY(this->P, winSize);
			printf_s("presum = %d totalsum = %d rotation count = %d\n\n", presum, totalsum, rocount);
		} while (abs(presum - totalsum) > 1);

		do {
			presum = totalsum;
			this->P = TranslateZ(this->P, winSize);
			printf_s("presum = %d totalsum = %d rotation count = %d\n\n", presum, totalsum, rocount);
		
		} while (abs(presum - totalsum) > 1);

		do {
			presum = totalsum;
			this->P = RotationY(this->P, winSize);
			printf_s("presum = %d totalsum = %d rotation count = %d\n\n", presum, totalsum, rocount);
		} while (abs(presum - totalsum) > 1);

		/*
		do {
			presum = totalsum;
			this->P = RotationX(this->P, winSize);
			printf_s("presum = %d totalsum = %d rotation count = %d\n\n", presum, totalsum, rocount);
		}while (abs(presum - totalsum) > 1);
		*/
		do {
			presum = totalsum;
			this->P = RotationZ(this->P, winSize);
			printf_s("presum = %d totalsum = %d rotation count = %d\n\n", presum, totalsum, rocount);
		} while (abs(presum - totalsum) > 1);
		rocount++;
		
	} while (abs(outerpresum - totalsum));
	//printf_s("presum = %d totalsum = %d rotation count = %d\n", presum, totalsum, rocount);
	return sum;
}

// 전체 프로세스 반복은 여기서 함수 하나 더 만들어주기.

Powell::Powell()
{
}


Powell::~Powell()
{
}
