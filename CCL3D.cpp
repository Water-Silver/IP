#include "CCL3D.h"

#include <stdio.h>
#include "CCL.h"
#include <vector>
#include <cstring>
#include <numeric>
#include <omp.h>
#include <algorithm>



#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))  
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))  

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



void CCL3D::InitMembers(short **image, int d, int w, int h) {
	this->image = image;
	this->depth = d;
	this->width = w;
	this->height = h;
	currentLabelCount = 1;


	printf_s("%d %d %d\n", this->depth, this->width, this->height);
	// this->count.assign(15000, 0); //이렇게 초기화 하는게 아닌가봐..
	
	for (int i = 0; i < 20000; i++) {
		count.push_back(0);
	}
}


/*          CCL             */

void CCL3D::firstpass() {
	
	for (int i = 0; i < depth; i++) {
		for (int j = 0; j < 512; j++) {
			for (int k = 0; k < 512; k++) {
				if(i==29 && j<3 &&k<3) printf_s("%d\t", image[i][k + j * width]);
			}
		}
	}
	
	printf_s("depth : %d", depth);
	label = new short*[depth];
	for (int i = 0; i < depth; i++) {
		label[i] = new short[width*height]; // are they initialized to zero? check
	}


	for (int d = 0; d < depth; d++) {
		for (int i = 0; i < 512 * 512; i++) {
			label[d][i] = 0;
		}
	}
	
	
	//////////////////////////eqv initialize
	eqv = new int [15000];
		for (int i = 0; i < 15000; i++) {
			eqv[i] = i;
		}
		
	/////////////////////////count vector initialize

		


	//vector <short> count(1000); //label에 포함된 픽셀 수 count


//////////////////////////////////////////////1 st pass //////////////////////////아직  boundary padding 안했음
		int numofpixvalue = 0;
		
	for (int d = 1; d < this->depth; d++) {
		for (int i = 1; i < this->height; i++) { // for every pixels except left end and up end
			for (int j = 1; j < this->width; j++) {
				int zlabel = 0;
				if (image[d][j + i * this->width] == 0)  continue;
	
				if (label[d][j + (i - 1)*this->width] == 0 && label[d][(j - 1) + i * this->width] != 0) { //#1 left has value
					if (label[d - 1][j + i * width] == 0) {
						label[d][j + i * this->width] = label[d][(j - 1) + i * this->width];
					}
					else { // 위에 값 있을 때 비교해서 작은 값 넣어줌
						if (label[d-1][j +i*width] < label[d][(j-1) + i*width]) {
							zlabel = label[d - 1][j + i * width];
							eqv[label[d][j - 1 + i * width]] = eqv[label[d - 1][j + i * width]];
							
						}
						else {
							zlabel = label[d][(j - 1) + i*width];
							eqv[label[d-1][j + i * width]] = eqv[label[d][j - 1 + i * width]];
							
						}
						label[d][j + i * this->width] = zlabel;

					}		
				}
				else if (label[d][j + (i - 1)*width] != 0 && label[d][(j - 1) + i * this->width] != 0) { // 왼 위 둘 다 차있을 때 더 작은 값을 취해야 #2
					if (label[d-1][j + i * width] == 0) { // depth -1 
						if (label[d][j + (i - 1)*width] < label[d][(j-1)+ i*width]) {
							zlabel = label[d][j + (i - 1)*width];
							eqv[label[d][(j - 1) + i * width]] = eqv[label[d][j + (i - 1)*width]];
							
						}
						else {
							zlabel = label[d][(j - 1) + i * this->width];
							eqv[label[d][j + (i - 1) * width]] = eqv[label[d][(j - 1) + i * width]];
						}
						label[d][j + i * this->width] = zlabel;
					}
					else { //위에도 값 있을 때 세 개 비교해야함. min 구해서 eqv  다 그거로 넣어주면 될거같은데

						short min = MIN(label[d][j + (i - 1)*width], label[d][(j - 1) + i * width]);
						min = MIN(min, label[d - 1][j + i * width]);

						zlabel = min;
						short eqvmin = eqv[min];
						eqv[label[d][j + (i - 1) * width]] = eqvmin;
						eqv[label[d][(j - 1) + i * width]] = eqvmin;
						eqv[label[d - 1][j + i * width]] = eqvmin;

						label[d][j + i * this->width] = zlabel;
						//eqv[label[d][j + i * this->width]] = eqvmin;

					}
					
				}
				else if (label[d][j + (i - 1)*width] != 0 && label[d][(j - 1) + i * this->width] == 0) { //#3 up has value
					if (label[d - 1][j + i * width] == 0) {
						label[d][j + i * this->width] = label[d][j + (i - 1) * this->width];
					}
					else { // 위에 값있을 때 비교해서 넣어줌.
						short min = MIN(label[d][j + (i - 1) *width], label[d - 1][j + i * width]);
						label[d][j + i * width] = min;
						int eqvmin = eqv[min];
						eqv[label[d][j + (i - 1)*width]] = eqvmin;
						eqv[label[d-1][j + i *width]] = eqvmin;

						
					}

				}
				else {// #4 left and up don't have value.
					//++currentLabelCount;
					if (label[d - 1][j + i * width] == 0) {
						//printf_s("?\n");
						label[d][j + i * this->width] = ++currentLabelCount;
						//eqv[label[d][j + i * width]] = currentLabelCount; 이미 이렇게  initialize 되어있음
						//if (currentLabelCount == 269) printf_s("%d %d %d here!!!!", d, i, j);
					}
					else { // depth 위에만 0 이 아닌 값이 있을 때
						label[d][j + i * this->width] = label[d-1][j  + i * this->width];
					}
				}



				// 이미 바꿔준 거를 다시 바꿀 때 해당 label 에 link??
				//if (eqv[355] !=269 && eqv[355] != 355) printf_s("%d %d %d %d\n", eqv[355], d, j, i);
				//if (eqv[355] != 7 &&eqv[355] != 355 ) printf_s("%d %d %d %d\n",eqv[355], d, j, i);
			}
		}

	//	if (d == 20) {
	//		printf_s("eqv table when d=20 print\n");
	//		for (int i = 0; i < 500; i++) {
	//			printf("eqv %d = %d\n", i, eqv[i]);
	//		}
	//	}

	//	if (d == 21) {
	//		printf_s("eqv table when d=21 print\n");
	//		for (int i = 0; i < 500; i++) {
	//			printf("eqv %d = %d\n", i, eqv[i]);
	//		}
	//	}

	}

	printf_s("currentLabel = %d\n", currentLabelCount);
	printf_s("%d", numofpixvalue);
	


	printf_s("\n\n");
	for (int d = 4; d < 7; d++) {
		for (int i = 250; i < 250 + 30; i++) { // for every pixels except left end and up end
			for (int j = 250; j < 250 + 30; j++) {
				printf_s("%d ", label[d][j + i * 512]); //////////////////////////////////////
			}
			printf_s("\n");
		}

		printf_s("\n\n");
	}

	printf_s("eqv table first print\n");
	for (int i = 0; i < 500; i++) {
		printf("eqv %d = %d\n", i, eqv[i]);
	}
	
	

}

void CCL3D::firstpassEcon() { // eight  connectivity


	label = new short*[this->depth]; // label   선언 해줘야 함.
	for (int i = 0; i < this->depth; i++) {
		label[i] = new short[this->width*this->height]; // are they initialized to zero? check
	}


	//debuggin 용  initialize
	/*
	for (int d = 0; d < depth; d++) {
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {

				label[d][j + i * width] = 0;// labelP[d + 1][(j + 1) + (i + 1)*widthP];

			}
		}
	}
	*/

	short con[3][3][3] = {
		{ {1,1,1}, {1,1,1}, {1,1,1} },
		{ { 1,1,1 }, { 1,0,0 }, { 0,0,0 }},  //////////////////////////
		{ {0, 0, 0}, { 0,0,0 }, { 0,0,0 } }
	};
	/*
	for (int l = -1; l <= 1; l++) {
		for (int n = -1; n <= 1; n++) {
			for (int m = -1; m <= 1; m++) {

				printf_s("%d \t", con[l + 1][n + 1][m + 1]);
				

			}
		}
	}
	*/
	int depthP = this->depth + 2;
	int widthP = this->width + 2;
	int heightP = this->height + 2;

	/////////////////////////////////////////Pad 된 레이블 생성
	
	short** labelP = new short*[depthP];
	for (int i = 0; i < depthP; i++) {
		labelP[i] = new short[widthP*heightP];

	}
	
	/*
	short** imageP = new short*[depthP];
	for (int i = 0; i < depthP; i++) {
		imageP[i] = new short[widthP*heightP];

	}
	*/
	// 생각해보니 레이블은 pad 할 필요가 없을거 같음.
	for (int d = 0; d < depthP; d++) {
		for (int i = 0; i < heightP; i++) {
			for (int j = 0; j < widthP; j++) {
				labelP[d][j+i*widthP] = 0;
			}
		}
	}
	/*
	for (int d = 0; d < depthP; d++) {
		for (int i = 0; i < heightP; i++) {
			for (int j = 0; j < widthP; j++) {
				imageP[d][j + i * widthP] = 0;
			}
		}
	}
	*/


	eqv = new int[25000];
	for (int i = 0; i < 25000; i++) {
		eqv[i] = i;
	}


	
	for (int d = 1; d < this->depth+1 ; d++) { //0~57 , 58 장
		for (int i = 1; i < this->width+1 ; i++) {
			for (int j = 1; j < this->height +1 ; j++) {


				//printf_s("currentLabel = %d d = %d j = %d i = %d\n", currentLabelCount,d,j,i);
				if (image[d-1][j-1 + (i-1) * this->width] == 0)  continue; // background 일 때 수행 안함.
				
				int min;
				vector<short> storeLabel(27);
				int num = 0;
				for (int p = -1; p <= 1; p++) {
					for (int n = -1; n <= 1; n++) {
						for (int m = -1; m <= 1; m++) {
							

							//printf_s("%d * %d d, j, i = %d %d %d\n", con[p +1][n+1][m+1], labelP[(d + p)][(j + m) + (i + n)*widthP], d+ p, j+m, i+n);
							if (con[p + 1][n + 1][m + 1] * labelP[(d + p)][(j +  m) + (i + n)*widthP] !=0) { // 안에 값이 있으면 벡터에 저장.
								storeLabel.at(num) = labelP[(d + p)][(j +  m) + (i + n)*widthP];
								num++;
								//printf_s("num ======%d\n", num);
								
							}

						}
					}
				}

				//printf_s("%d\n", num);
				if (num != 0) {
					vector<short> index(num); int k = 0;
					for (auto i : sort_indexes(storeLabel)) { // 들어 있는 값이 큰 순서대로 인덱스를 반환하는 함수
						if (i == 0) break;
						index.at(k++) = i;
					}
						//sort_indexes(store);
						//if (fChamfer[d+acheck][j+ccheck + (i+bcheck)*width] != 255) {
					
					int idx = index.back();
					min = storeLabel.at(idx);

					labelP[d][j + i*widthP] = min; 
					//printf_s("inint label[%d][%d +  %d*widthP] = %d\n", d, j, i, min);
					int eqvmin = eqv[min];

					for (int i = 0; i < num; i++) {
						int naver = storeLabel.at(i);

						int xRoot = find(min);
						int yRoot = find(naver); // neighbor 의 최종 label

						if (xRoot == yRoot) {}
						else {
							if ( yRoot > xRoot) { // 여기 원래는 사이즈 비교인데 그냥 더 작은 값을 갖는다는 의미로 써봄.
								eqv[naver] = xRoot;
							}
							else {
								eqv[xRoot] = yRoot;
							}
						}
					}

				}
				else if( num==0){ 
					labelP[d][j+i*widthP] = ++currentLabelCount; 
					//printf_s(" init new label[%d][%d +  %d*widthP] = %d\n", d, j, i, currentLabelCount);
				}
				

			}

		}
		printf_s("depth = %d\n", d);
		
	}
	

	for (int d = 0; d < depth; d++) {
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {

				label[d][j + i * width] = labelP[d + 1][(j + 1) + (i + 1)*widthP];

			}
		}
	}

	printf_s("\n\n");
	for (int d = 0; d < 5; d++) {
		for (int i = 250; i < 250 + 30; i++) { // for every pixels except left end and up end
			for (int j = 250; j < 250 + 30; j++) {
				printf_s("%d ", label[d][j + i * 512]); //////////////////////////////////////
			}
			printf_s("\n");
		}

		printf_s("\n\n");
	}

	


}



short CCL3D::find(short a) { // function to find the minimal equvialent label
	

	int eqvA = eqv[a];
	int tempe;
		while (eqvA != a) {

			tempe = eqv[a];
			eqvA = eqv[tempe];
			a = tempe;

		}


		return  a;


}

void CCL3D::Rootunion(short a, short b) {
	short aRoot = find(a);
	short bRoot = find(b);

	if (aRoot == bRoot) return;

	if (count[aRoot] < count[bRoot]) {
		short tempA = aRoot;
		aRoot = bRoot;
		bRoot = tempA;

		eqv[bRoot] = aRoot;

		count[aRoot] = count[aRoot] + count[bRoot];
	}

}





void CCL3D::secondpass() {

	



	///////////////////////////2nd 패쓰  equivalency list while 문 돌려서 linking
	for (int d = 0; d < depth; d++) {
		for (int i = 1; i < this->height; i++) { // for every pixels except left end and up end
			for (int j = 1; j < this->width; j++) {
				if (label[d][j + i * this->width] != 0) {
					// process if background		   
					label[d][j + i * this->width] = find(label[d][j + i * this->width]);
					
					count[label[d][j + i * this->width]]++;


				}
			}
		}
	}

	//background removal

	count[label[0][0]] = 0;
	count[label[0][512]] = 0;
	for (int i = 0; i < 512; i++) {
		count[label[0][0]] = 0;

		count[label[0][512*i]] = 0; //왼쪽 끝점
		count[label[0][512 * i + 511]] = 0; //오른쪽 끝점

	}
	
	




	/*
	printf_s("\n");
	for (int i = 512 /2 - 13; i < 512 / 2 + 13; i++) { // for every pixels except left end and up end
		for (int j = 512 / 2 - 13; j < 512 / 2 + 13; j++) {
			printf_s("%d ", label[39][j + i * 512]); //////////////////////////////////////
		}
		printf_s("\n");
	}

	printf_s("\n\n");
	*/
	printf_s("\n\n");
	for (int d = 4; d < 9; d++) {
		for (int i = 250; i < 250 + 30; i++) { // for every pixels except left end and up end
			for (int j = 250; j < 250 + 30; j++) {
				printf_s("%d ", label[d][j + i * 512]); //////////////////////////////////////
			}
			printf_s("\n");
		}

		printf_s("\n\n");
	}

	

	/*
	for (int i = 0; i < 1000; i++) {
		printf("eqv %d = %d\n", i, eqv[i]);
	}
	*/
}



// 여기서 count[] 를 sort
void CCL3D::visualize() {
	
	image_vi = new short*[this->depth]; // initialization of the array to be returned
	for (int i = 0; i < this->depth; i++) {
		image_vi[i] = new short[this->width*this->height];
	}
	

	//short*image2d = this->image[3];
	/*
	for (int i = 512 / 4 - 5; i < 512 / 4 + 5; i++) { // for every pixels except left end and up end
		for (int j = 512 / 4 - 5; j < 512 / 4 + 5; j++) {
			printf_s("%d ", image2d[j+i*512]); //////////////////////////////////////
		}
		printf_s("\n");
	}
	
	printf_s("\n\n");

	*/


	vector <int> size(20000);
	for (int i = 0; i < 20000; i++) {
		size[i] = 0;
	}

	




	int k = 0;
	for (auto i : sort_indexes(this->count)) { // 사이즈가 큰 순서대로 레이블을 저장
		size[k++] = i;
		
	}
	k = 0;
	for (int i = 0; i<10; i++) {
		printf_s("%d st large = index = %d count = %d\n", k, size[k], count[size[k]]);
		k++;
	}




	for (int d = 0; d < this->depth; d++) {
		
		k = 0;
		for (int i = 0; i < this->height ; i++) { // for every pixels except left end and up end //사이즈가 큰 레이블에 해당되는 픽셀만 255 나머지는 0 으로  이진화
			for (int j = 0; j < this->width ; j++) {
				if ( label[d][j + i * this->width] == size[0]) {
					image_vi[d][j + i * width] = 255;
					//printf_s("in here?\n");
				}
				else  image_vi[d][j + i * width] = 0;
				//if(d== 39) printf_s("%d\n", label[d][j + i*width]);
			}
		}
	}
}



CCL3D::CCL3D()
{
}


CCL3D::~CCL3D()
{
}
