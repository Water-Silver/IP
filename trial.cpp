#include <opencv2/opencv.hpp>
#include <iostream>
#include "../Common/memory.h"
#include "../Common/image3d.h"
#include "../Core/raw_io.h"
#include "../Core/raw_io_exception.h"
#include <memory>
//#include <iostream>
#include <vector>
#include <algorithm>
#include <utility>
#include <limits>
#include <fstream>
#include <ctime>
#include <omp.h>
#include <vector>
#include <numeric>
#include <stdio.h>
#include <fstream>
#include "CCL3D.h"
#include <chrono>
#include "Points.h"
#include "Powell.h"


using namespace std;


template <class T>
std::unique_ptr<mc::image3d<T>> load_image(const std::string& path, const unsigned int w, const unsigned int h, const unsigned int d)
{
	using namespace mc;
	image3d<T>* pImg = nullptr;

	try {
		raw_io<T> io(path.c_str());
		io.setEndianType(raw_io<T>::EENDIAN_TYPE::BIG);
		pImg = io.read(w, h, d);
	}
	catch (raw_io_exception& e) {
		std::cerr << e.what() << std::endl;
	}

	return std::make_unique<image3d<T>>(*pImg);
}

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

void writeFile(short **image_3d, int size, char filename[]) {   // this function writes a binary file given array of image sets, size and the name of the new file.


	short *buffer = new short[size / 2];
	short *buffer2 = new short[size / 2];
	int depth = size / (512 * 512);
//	cout << depth << endl;
	int p = 0;
	for (int i = 0; i < depth / 2; i++) {
		for (int j = 0; j < 512; j++) {
			for (int k = 0; k < 512; k++) {
				buffer[p] = image_3d[i][k + j * 512];
				//if (i == 0 && k > 0 && k < 3 && j < 3) cout << image_3d[i][k + j * 512] << endl;
				p++;
				//cout << p << endl;
			}
		}
	}
	cout << "\n" << endl;
	p = 0;
	for (int i = depth / 2; i < depth; i++) {
		for (int j = 0; j < 512; j++) {
			for (int k = 0; k < 512; k++) {
				buffer2[p] = image_3d[i][k + j * 512];
			 	//if (i == depth / 2 && k > 0 && k < 3 && j < 3) cout << image_3d[i][k + j * 512] << endl; // 출력해서 결과 다르게 나오는거 확인용
				p++;
				//cout << p << endl;
			}
		}
	}


	ofstream myfile;
	myfile.open(filename, ios::out | ios::binary );
	if (!myfile.is_open()) {
		cout << "cannot open file" << endl;
	}
	myfile.write((char *)&(*buffer), 512 * 512 * depth);
	myfile.write((char *)&(*buffer2), 512 * 512 * depth);

	myfile.close();
	//cout << &image_3d << endl;

	//free(&buffer);
	//free(&myfile);

	/*
	cv::Mat image = cv::Mat(512 * 512, 1, CV_16U, image_3d[45]);
	cv::Mat reshaped = image.reshape(0, 512);
	cv::imwrite("cvwrite.jpg", reshaped);
	*/

}

short **readFile( char filename[],  int depth) {

	//std::unique_ptr<mc::image3d<short>> image = load_image<short>(filename, width, height, depth);
	
	short *buffer = new short[512 * 512 * depth/2];
	short *buffer2 = new short[512 * 512 * depth/2];
	ifstream myFile(filename, ios::in | ios::binary);
	myFile.read((char *)&(*buffer), 512 * 512 * depth);
	myFile.read((char*)&(*buffer2), 512 * 512 * depth);


	///////////////////////////////////////////////////////initialize image2_nr using two buffers. /////여기 depth 내가 일일이 바꿔줘야 함.

	short** newImage = new short*[depth]; // initialization of the array to be returned
	for (int i = 0; i < depth; i++) {
		newImage[i] = new short[512*512];
	}


	//cout << "done!" << endl;
	int p = 0;
	for (int i = 0; i < depth / 2; i++) {
		for (int j = 0; j < 512; j++) {
			for (int k = 0; k < 512; k++) {
				newImage[i][k + j * 512] = buffer[p];
				p++;
				//if (i > 29 && i < 31) printf_s(" %d %d\n", buffer[p], image2_nr[i][k + j * width]);
			}
		}
	}
	p = 0;

	for (int i = depth / 2; i < depth; i++) {
		for (int j = 0; j < 512; j++) {
			for (int k = 0; k < 512; k++) {
				newImage[i][k + j * 512] = buffer2[p];
				p++;
				//if (i > 29 && i < 31) printf_s(" %d %d\n", buffer2[p], image2_nr[i][k + j * width]);
			}
		}
	}

	return newImage;

}


short** threshold(int depth, int width, int height, short **image_3d) {

	cout << depth << endl;
	short** image = new short*[depth]; // initialization of the array to be returned
	for (int i = 0; i < depth; i++) {
		image[i] = new short[width*height];
	}
	for (int d = 0; d < depth; d++) {
		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {


				if (image_3d[d][i + j * width] <-400 && image_3d[d][i + j * width] > -1024) {
					image[d][i + j * width] = 255;
					//cout << image[d][i + j * width] << endl;
				}
				else {
					//image_array[i] = -image_array[i] / 1024 * 225; 
					image[d][i + j * width] = 0;
				}
			}
		}
	}
	return image;
}

short** noiseRemoval(int depth, int width, int height, short **image_3d) {
	
	short** image = new short*[depth]; // initialization of the array to be returned
	for (int i = 0; i < depth; i++) {
		image[i] = new short[width*height];
	}
	int n = 1;
	for (int d = 0; d < depth; d++) {
		for (int j = 0; j < height; j++) {
			for (int i = 0; i <width; i++) {
				int sum = 0; 
				vector <short> kernel(9);


				int count2 = 0; int median;
				for (int x = -n; x <= n; x++) { // for each kernel window
					for (int y = -n; y <= n; y++) {

						if ((i + x <= width- 1) && (i + x >= 0) && (j + y <= height - 1) && (j + y >= 0)) {
							kernel[count2] = image_3d[d][ (i+x) + (j+y) * width];
							count2++;
						}
					}
					
				}
				
				median = count2 / 2;
				std::sort(kernel.begin(), kernel.end());
				image[d][i+j*width] = kernel.at(median);
				
			}
		}
	}
	return image;
}


short ** edgeDetect(int depth, int width, int height, short **image_3d) {
	short kernel2[3][3] = { { 0,1,0 }, { 1,0,1 }, { 0,1,0 } }; //////////////////////////


	short** image = new short*[depth]; // initialization of the array to be returned
	for (int i = 0; i < depth; i++) {
		image[i] = new short[width*height];
	}

	for (int d = 0; d < depth; d++) {
		for (int i = 0; i < width ; i++) {
			for (int j = 0; j < height ; j++) {
				image[d][j + i * width] = 0;
			}
		}
	}



	for (int d = 0; d < depth; d++) {
		for (int i = 2; i < width - 2; i++) {
			for (int j = 2; j < height - 2; j++) {
				int sum = 0;
				for (int n = -1; n <= 1; n++) {
					for (int m = -1; m <= 1; m++) {

						sum += kernel2[1 + n][1 + m] * image_3d[d][(j+m)+(i+n)*width];
						//noise += kernel2[1 + n][1 + m] * image_2dCopy[i + n][j + m];
					}
				}
				
				if (image_3d[d][j+i*width] == 255 && sum < 255 * 4) { image[d][j+i*width] = 255; }
				else  image[d][j+i*width] = 0;
			


				// just for visualizing center of inertia
				//if ((abs(i - 267) < 5 && abs(j - 150) < 5) || (abs(i - 281) < 5 && abs(j - 370) < 5)) {
				//	image_2dCopy3[i][j] = 255;}
				}
			}
		}
	return image;
}


using namespace std;
using namespace std::chrono;
short** ChamferDistance(int o_depth, int o_width, int o_height, short**image_3d) {

	int depth = o_depth + 2;
	int width = o_width + 2;
	int height = o_height + 2;
	

	//space to save intermediate values.
	short** fChamfer = new short*[depth]; // initialization of the array to be returned
	short** bChamfer = new short*[depth];
	short** bChamfer2 = new short*[depth];
	for (int i = 0; i < depth; i++) {
		fChamfer[i] = new short[width*height];
		bChamfer[i] = new short[width*height];
		bChamfer2[i] = new short[width*height];
	}

	short** writeChamfer = new short*[o_depth];
	for (int i = 0; i <o_depth; i++) {
		writeChamfer[i] = new short[o_width*o_height];

	}


	//define 2 kernel, forward and backward
	short forward[3][3][3] = {
		{ {1,1,1}, {1,1,1}, {1,1,1} },
		{ { 1,1,1 }, { 1,0,0 }, { 0,0,0 }},  //////////////////////////
		{ {0, 0, 0}, { 0,0,0 }, { 0,0,0 } }
	};

	short backward[3][3][3] = {
	{ {0, 0, 0}, { 0,0,0 }, { 0,0,0 } },
	{ { 0,0,0 }, { 0,0,1 }, { 1,1,1 }},  //////////////////////////
	{ {1,1,1}, {1,1,1}, {1,1,1} },
	};

	//initialize edges to 0 in fChamfer and bChamfer
	for (int d = 0; d < depth; d++) {
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				if (d == 0 || d == depth - 1) {
					fChamfer[d][j + i * width] = 255; // max pad
					bChamfer[d][j + i * width] = 255;
					bChamfer2[d][j + i * width] = 255;
				}
				else if( (i==0 || i==height-1)  || (j == 0 || j == width - 1)) {
					fChamfer[d][j + i * width] = 255; // max pad
					bChamfer[d][j + i * width] = 255;
					bChamfer2[d][j + i * width] = 255;
					
				}
				else { // 근데 이 부분 없어도 될 것 같기도. 밑에서 다 접근 하는듯?
					if (image_3d[d-1][j-1 + (i-1) * o_width] == 255) { //edge -> surface (distance 0) // 여기서 오류 왜 나지.

					fChamfer[d][j + i * width] = 0;
					bChamfer[d][j + i * width] = 0;
					bChamfer2[d][j + i * width] = 0;
					}
					else {
						fChamfer[d][j + i * width] = 255;
						bChamfer[d][j + i * width] = 255;
						bChamfer2[d][j + i * width] = 255;
					}
				}
			}
		}
	}
	
	cout << "" << endl;

	///////////////////////////////first pass


	for (int d = 0; d < o_depth; d++) { //0~57 , 58 장
		for (int i = 0; i < o_width - 1; i++) {
			for (int j = 0; j < o_height - 1; j++) {

				if (image_3d[d][j + i * o_width] == 255) fChamfer[d+1][j+1 + (i+1) * width] = 0;
				else {

					int min;
					vector<int> store(13, 255);
					int num = 0;
					for (int l = -1; l <= 1; l++) {
						for (int n = -1; n <= 1; n++) {
							for (int m = -1; m <= 1; m++) {

								if (forward[1 + l][1 + n][1 + m] != 0) {
									store.at(num) = fChamfer[(d+1) + l][(j +1+ m) + (i +1+ n)*width];
									num++; //num 이 increment 가 안됨 ;;

								}

								//store.at(num++) = forward[1 + l][1 + n][1 + m] * image_3d[d + l][(j + m) + (i + n)*width];

							}
						}
					}
					//cout << "num coming" << endl;
					//if (num != 0) printf_s("num = %d\n", num);


					//high_resolution_clock::time_point t1 = high_resolution_clock::now();


					if (num != 0) {
						vector<int> index(13); int k = 0;
						for (auto i : sort_indexes(store)) { // 들어 있는 값이 큰 순서대로 인덱스를 반환하는 함수
							index.at(k++) = i;
						}
						//sort_indexes(store);
						//if (fChamfer[d+acheck][j+ccheck + (i+bcheck)*width] != 255) {



						int idx = index.back();
						//if (fChamfer[d + acheck][j + ccheck + (i + bcheck)*width] != 255)	printf_s("%d", idx);


						min = store.at(idx);
						
						//if (fChamfer[d + acheck][j + ccheck + (i + bcheck)*width] != 255)printf_s("min = %d ", min);
						if (min == 255) fChamfer[d+1][j+1 + (i+1) * width] = 255;
						else fChamfer[d+1][(j+1) + (i+1) * width] = min + 1; // 1~58 까지 써짐.  59 는 그대로 255로 채워져있단 뜻
					}
				}

			}
		
		}
		cout << d << endl;
	}
	

	
	///////////////////////////////second pass

	for (int d = o_depth-1; d >=0; d--) { // d = 57; d>=0; d--; 57~0 58장.
		for (int i = o_height - 1; i >= 0; i--) { // i = 511; i>=0;i--;
			for (int j = o_width - 1; j >= 0; j--) {
				int min;
				vector<short> store(13, 255);
				int num = 0;

				if (image_3d[d][j + i * o_width] == 255) bChamfer[d+1][j+1 + (i+1) * width] = 0; // 57-> 58
				else {
					for (int l = -1; l <= 1; l++) {
						for (int n = -1; n <= 1; n++) {
							for (int m = -1; m <= 1; m++) {

								if (backward[1 + l][1 + n][1 + m] != 0) {
									store.at(num) = bChamfer2[d +1+ l][(j+1 + m) + (i+1 + n)*width]; // 58 +-1
									num++;
								}


							}
						}
					}


					if (num != 0) {
						vector<int> index(13); int k = 0;
						for (auto i : sort_indexes(store)) { // 들어 있는 값이 큰 순서대로 인덱스를 반환하는 함수
							index.at(k++) = i;
						}
						//sort_indexes(store);

						int idx = index.back();
						//printf_s("%d", idx);


						min = store.at(idx);
						if (min == 255) bChamfer[d+1][j+1 + (i +1)* width] = 255;
						else bChamfer[d+1][(j+1) + (i+1) * width] = min + 1;

					}

					min = MIN(fChamfer[d+1][j+1 + (i+1) * width], (min + 2)); //58
					//if (d == o_depth-1) printf_s("fChamfer = %d bChamfer = %d min = %d\n", fChamfer[d+1][j+1 + (i+1) * width], bChamfer[d+1][j+1 + (i+1) * width], min);
					if (min == 255) bChamfer2[d+1][j+1 + (i+1) * width] = 255;
					else bChamfer2[d+1][j+1 + (i+1) * width] = min;

				}
			}
		}
		cout << d << endl;
	}
	

	

for (int d = 0; d < o_depth; d++) {
	for (int i = 0; i < o_height; i++) {
		for (int j = 0; j < o_width; j++) {

			writeChamfer[d][j + i * o_width] =  bChamfer2[d + 1][(j + 1) + (i + 1)*width];

		}
	}
}

	return writeChamfer ;
}



/*

Powell's method idea.
일단  3 차원 공간을 만든다음에 안에다가
distance map 채워넣음

Point 라는 클래스 만들어서 floating image 의 edge 에 해당하는 점들의 index (d,w,h) 를 저장.
Transform matrix 를 Tx, Ty, Tz, Rx, Ry, Rz 설계해서 ( 얼마씩 increment 혹은 decrement 할지는 내가 정함)
transform 된 점 P' 를 구하고. 그 때의 d', w', h' 를 구해서
distance map 의 범위 안에 있으면 sum 을 increment 밖이면 sum 에 최대값 increment.
가장 작은  distance map sum 방향으로 이동.
Tx -> Ty -> Tz -> Rx -> Ry -> Rz 이 루틴으로 반복하고. 앞으로 다시 돌아가서 전 과정 다시 수행.

값이 더 이상 변하지 않으면 stop


*/





vector<Points> storePoints(short **edgeImg, int d, int x, int y, int z) {
	int depth = d;
	int width = 512; 
	int height = 512;
	//COI w h d 252 277 28
	vector <Points> pt;
	for (int d = 0; d < depth; d++) {
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				if (edgeImg[d][j + i * width] == 255) {
					Points p;
					p.Init(d-z, j-x, i-y);
					pt.push_back(p);
				}

			}
		}
	}

	// store edge points in vector
	return pt;

}



short** Interpolation(short **image, int depth, int w, int h) {

	int newDepth = depth + 4 * (depth - 1);
	int width = w;
	int height = h;

	short **newImg = new short*[newDepth]; // 공간 생성
	for (int i = 0; i < newDepth; i++) {
		newImg[i] = new short[width * height];
	}


	for (int d = 0; d < newDepth; d++) {
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				if (d % 5 == 0) {
					newImg[d][j + i * 512] = image[d/5][j + i * 512];
				}
				else {
					int up = d / 5;
					int below = up + 1;
					int idx = d % 5;
					newImg[d][j + i * 512] = (image[up][j + i * 512] * (5 - idx) + image[below][j + i * 512] * idx) / 5;
				}

			}
		}
	}


	return newImg;

}




// check if there is value in the certain area and if there is, put them in a vector, find min and put it back in the center.
//this process is done 2 times for forward pass and backward pass.


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main()
{

	// min = -1024, max = 1628  
	using namespace mc;
	using namespace cv;
	int s;
	std::unique_ptr<mc::image3d<short>> img1 = load_image<short>("volume1_512x512x56.raw", 512, 512, 56);
	std::unique_ptr<mc::image3d<short>> img2 = load_image<short>("volume2_512x512x58.raw", 512, 512, 58);
	//auto Image1 = new image3d<short>(512, 512, 1);
	//auto Image2 = new image3d<short>(512, 512, 1);
	//printf("%d\n", img1->get(1, 1, 1)); // 
	//printf("%d\n", img1->get(2, 1, 1)); //  두 번째 슬라이스의 1 * 1 번째 픽셀의 값(인텐시티)

	/*
	short * image_array = img1->data(36); // 36 번째 슬라이스 (이미지) 의 한 줄로 펼쳐진 배열
	short * image_array2nd = img2->data(36); // 36 번째 슬라이스 (이미지) 의 한 줄로 펼쳐진 배열
	*/


	//naming rule
	short **image_3d_raw = img1->data();
	short **image_3d_2_raw = img2->data();


	int depth = img1->depth();
	//cout << depth << endl;
	int depth_2 = img2->depth();
	int width = img1->width();
	int height = img1->height();

	
	int newDepth1 = depth + 4 * (depth - 1);
	short **image_3d = new short*[newDepth1]; // 공간 생성
	for (int i = 0; i < newDepth1; i++) {
		image_3d[i] = new short[width * height];
	}
	image_3d = Interpolation(image_3d_raw, depth, width, height);
	
	int newDepth2 = depth_2 + 4 * (depth - 1);
	short **image_3d_2 = new short*[newDepth2]; // 공간 생성
	for (int i = 0; i < newDepth2; i++) {
		image_3d_2[i] = new short[width * height];
	}

	
	image_3d_2 = Interpolation(image_3d_2_raw, depth_2, width, height);

	
	//writeFile(newImg, newDepth1 * 512 * 512, "SliceInterpolate1.raw");
	//writeFile(newImg2, newDepth2 * 512 * 512, "SliceInterpolate2.raw");

	///////////  slice interpolation 후 수정 사항
	//short ** image_3d = newImg; // 잘 들어가 있는거 확인 했음

	//short** image_3d_2 = newImg2;
	
	depth = newDepth1;
	depth_2 = newDepth2;
	
	cout << depth << endl;

	// TODO #1 : segmentation of lung region. (use thresholding, CCA)
	// thresholding -> in to binary values
	
	
	//short ** image1_th =  threshold(depth, width, height, image_3d);
    //short ** image2_th = threshold(depth_2, width, height, image_3d_2);
    
	//writeFile(image1_th, depth * 512 * 512, "threshold1.raw");
    //writeFile(image2_th, depth_2 * 512 * 512, "threshold2.raw");
	


    //short **image1_nr = noiseRemoval(depth, 512, 512, image1_th);
	//short **image2_nr = noiseRemoval(depth_2, 512, 512, image2_th);
	//writeFile(image1_nr, depth * 512 * 512, "noiseRemoved1.raw");
	//writeFile(image2_nr, depth_2 * 512 * 512, "noiseRemoved2.raw");

   ////////////////////////////////////////////////////////////////////////////////////  CCL 


	
	//short **image1_nr = readFile("noiseremoved1.raw", depth);
	//short **image2_nr = readFile("noiseremoved2.raw", depth_2);
	
	/*
	CCL3D CLA;
	CLA.InitMembers(image1_nr, depth, width, height);
	
	CLA.firstpassEcon();
	writeFile(CLA.label, depth * 512 * 512, "ccl1stpass.raw");
	CLA.secondpass();
	CLA.visualize();

	
	CCL3D CLA2;
	CLA2.InitMembers(image2_nr, depth_2, width, height);

	CLA2.firstpassEcon();
	writeFile(CLA2.label, depth_2 * 512 * 512, "ccl2_1stpass.raw");
	CLA2.secondpass();
	CLA2.visualize();
	cout << "done!" << endl;
	

	
	
	writeFile(CLA.label, depth * 512 * 512, "ccllabel_1.raw");
	writeFile(CLA.image_vi , depth * 512 * 512, "cclresult_1.raw");

	short**edgeimg = edgeDetect(depth, width, height, CLA.image_vi);
	writeFile(edgeimg, depth * 512 * 512, "edgeimage_1.raw");



	writeFile(CLA2.label, depth_2 * 512 * 512, "ccllabel_2.raw");
	writeFile(CLA2.image_vi, depth_2 * 512 * 512, "cclresult_2.raw");

	short**edgeimg2 = edgeDetect(depth_2, width, height, CLA2.image_vi);
	writeFile(edgeimg2, depth_2 * 512 * 512, "edgeimage_2.raw");
	
	*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	
	short** CLA1 = readFile("cclresult_1.raw", depth);
	short** CLA_2 = readFile("cclresult_2.raw", depth_2);

	int center_x = 0;
	int center_y = 0;
	int center_z = 0;
	int countcoi = 0;

	for (int d = 0; d < depth; d++) {
		for (int i = 1; i < height; i++) { // for every pixels except left end and up end //사이즈가 큰 레이블에 해당되는 픽셀만 255 나머지는 0 으로  이진화
			for (int j = 1; j < width; j++) {
				if (CLA1[d][j + i * 512] == 255) {
					center_y += i;
					center_x += j;
					center_z += d;
					countcoi++;
				}
			}
		}
	}
		center_x /= countcoi;
		center_y /= countcoi;
		center_z /= countcoi;
	

		printf_s("%d %d %d\n", center_x, center_y, center_z);


	
	//short** edgeimg = readFile("edgeimage_2.raw", depth_2);
	

	//short**distancemap = ChamferDistance(depth_2, width, height, edgeimg2);  ////// 여기  edgeimg2 들어간거 기억하기

	//writeFile(distancemap, depth_2 * 512 * 512, "distancemap.raw");

	

	short** ChamferImage = readFile("distancemap.raw", depth_2);
	//writeFile(ChamferImage, depth_2 * 512 * 512, "CheckChamferRead.raw");


//	여기서  storepoints 라는 함수 호출해서 edgeimg1  에서 에지로 검출된 점들의 좌표를  point 클래스 인자로 넘겨주고  point 클래스 담은 벡터를 만들 것.

//
//
///* ////////////////////////////////////////////////////////Registration



	short** edgeImg1 = readFile("edgeimage_1.raw", depth);
	Points p;
	vector <Points> P;
	P = storePoints(edgeImg1, depth, center_x, center_y, center_z);
	Powell PowMethod(ChamferImage, edgeImg1, P, depth, depth_2, center_x, center_y, center_z);
	//PowMethod.InitMembers(ChamferImage, edgeImg1, P);


	PowMethod.PowIterNew(ChamferImage, edgeImg1); // 여기 원래 parser?? 이거 필요 없어도 되는데

	// image transform 

	Mat T(4, 4, CV_64FC1);
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			T.at<double>(i, j) = PowMethod.T[i][j];
		}
	}
	Mat Tinv = T.inv();

	printf("T = \n");
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			printf_s("%lf ", T.at<double>(i, j));
		}
		printf_s("\n");
	}
	
	printf("Tinv = \n");
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			printf_s("%lf ", Tinv.at<double>(i, j));
		}
		printf_s("\n");
	}
	PowMethod.T;



	// 여기서  inverse warping 해서 넣어주기.
	printf_s("h = %lf\n", Tinv.at<double>(3, 0) + Tinv.at<double>(3, 1) + Tinv.at<double>(3, 2) + Tinv.at<double>(3, 3));



	//
	/*
	int newDepth1 = depth + 4 * (depth - 1);
	short **newImg = new short*[newDepth1]; // 공간 생성
	for (int i = 0; i < newDepth1; i++) {
		newImg[i] = new short[width * height];
	}
	newImg = Interpolation(image_3d, depth, width, height);

	int newDepth2 = depth_2 + 4 * (depth_2 - 1);
	short **newImg2 = new short*[newDepth2]; // 공간 생성
	for (int i = 0; i < newDepth2; i++) {
		newImg2[i] = new short[width * height];
	}
	newImg2 = Interpolation(image_3d_2, depth_2, width, height);
	depth = newDepth1;
	depth_2 = newDepth2;

	*/
	short** Registered = new short*[depth_2]; // initialization of the array to be returned
	for (int i = 0; i < depth_2; i++) {
		Registered[i] = new short[512 * 512];
	}

	short** BeforeRegistration = new short *[depth_2];
	for (int i = 0; i < depth_2; i++) {
		BeforeRegistration[i] = new short[512 * 512];
	}

	short** InvTransformed = new short *[depth_2];
	for (int i = 0; i < depth_2; i++) {
		InvTransformed[i] = new short[512 * 512];
	}

	for (int z = 0; z < depth; z++) { // coordinate system of img2. about to subtract img1 from img2 ( img2-img1)
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {


				//interpolate!!  trilinear interpolation
				double i = Tinv.at<double>(0, 0)*x + Tinv.at<double>(0, 1)*y + Tinv.at<double>(0, 2)*z + Tinv.at<double>(0, 3); // width
				double j = Tinv.at<double>(1, 0)*x + Tinv.at<double>(1, 1)*y + Tinv.at<double>(1, 2)*z + Tinv.at<double>(1, 3); // height
				double d = Tinv.at<double>(2, 0)*x + Tinv.at<double>(2, 1)*y + Tinv.at<double>(2, 2)*z + Tinv.at<double>(2, 3); // depth

				
				//printf_s("z = %d d = %lf\n", z, d);

				BeforeRegistration[z][x + y * 512] = image_3d_2[z][x + y * 512]- image_3d[z][x + y * 512];

				

					int fd = (int)floor(d);
					int fi = (int)floor(i);
					int fj = (int)floor(j);
					int cd = (int)ceil(d);
					int ci = (int)ceil(i);
					int cj = (int)ceil(j);
					if (fd > 0 && cd < depth && fi >0 && ci < 512 && fj >0 && cj < 512) {
						double C000 = image_3d[fd][ci + fj * 512];
						double C001 = image_3d[cd][ci + fj * 512];
						double C010 = image_3d[fd][fi + fj * 512];
						double C011 = image_3d[cd][fi + fj * 512];
						double C100 = image_3d[fd][ci + cj * 512];
						double C101 = image_3d[cd][ci + cj * 512];
						double C110 = image_3d[fd][fi + cj * 512];
						double C111 = image_3d[cd][fi + cj * 512];

						double lamda = i - floor(i); // width
						double mu = j - floor(j); // height
						double tri = d - floor(d); //depth

						double C00 = C000 * (1 - lamda) + C100 * lamda;
						double C01 = C001 * (1 - lamda) + C101 * lamda;
						double C10 = C010 * (1 - lamda) + C110 * lamda;
						double C11 = C011 * (1 - lamda) + C111 * lamda;

						double C0 = C00 * (1 - mu) + C10 * (mu);
						double C1 = C01 * (1 - mu) + C11 * (mu);

						double C = C0 * (1 - tri) + C1 * tri;
						InvTransformed[z][x + y * 512] = C;
						Registered[z][x + y * 512] = image_3d_2[z][x + y * 512] - C;// image_3d_2[d][i + j * 512];// -image_3d[d][i + j * 512];
						//printf_s("%d\n", Registered[z][x + y * 512]);
					}
				//printf_s("%d\n", image1_nr[d][i + j * 512]);

			}
		}
	}

	writeFile(Registered, depth_2 * 512 * 512, "registeredwithTriInterpolation.raw");
	writeFile(BeforeRegistration, depth_2 * 512 * 512, "BeforeRegistration.raw");
	writeFile(InvTransformed, depth_2 * 512 * 512, "InverseTransformed.raw");

	





	cout << "j" << endl;
	scanf_s("%d", &s);
	waitKey(0);
	return EXIT_SUCCESS;
}

