#include <stdio.h>
#include "CCL.h"
#include <vector>
#include <cstring>
#include <numeric>
#include <omp.h>
#include <algorithm>
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

void CCL::InitMembers(short **image, int d, int w, int h) {
		this->image = image;
		this->depth = d;
		this->width = w;
		this->height = h;

}


	/*          CCL             */

void CCL::firstpass() {

	    label = new short*[depth];
		for (int i = 0; i < depth; i++) {
			label[i] = new short[width*height]; // are they initialized to zero? check
		}

		/*
		for (int i = 0; i < 512; i++) {
			for (int j = 0; j < 512; j++) {
				label[i][j] = 0;
			}
		}
		*/
		int zlabel = 0;
		//////////////////////////eqv initialize
		eqv = new short *[depth];
		for (int d = 0; d < this->depth; d++) {
			eqv[d] = new short[1000];
		}
		for (int d = 0; d < this->depth; d++) {
			for (int i = 0; i < 1000; i++) {
				eqv[d][i] = i;
			}
		}
		/////////////////////////count vector initialize
		for (int d = 0; d < this->depth; d++) {
			vector<short> element(depth);
			count.push_back(element);

		}

		for (int d = 0; d < this->depth; d++) {
			for (int i = 0; i < 1000; i++) {
				count[d][i] = 0;
			}

		}

		//vector <short> count(1000); //label�� ���Ե� �ȼ� �� count


	//////////////////////////////////////////////1 st pass //////////////////////////����  boundary padding ������

		for (int d = 0; d < this->depth; d++) {
			for (int i = 1; i < this->height; i++) { // for every pixels except left end and up end
				for (int j = 1; j < this->width; j++) {
					if (image[d][j + i * this->width] != 0)  continue;

					// process if background
					//first check the neighbors
					//image_2d[i-1][j] image_2d[i][j-1] �� ���̺��� �ñ�. ó���� 0 ���� ����


					if (label[d][j + (i - 1)*this->width] == 0 && label[(j - 1) + i * this->width] != 0) { //#1
						label[d][j + i * this->width] = label[d][(j - 1) + i * this->width];
					}
					else if (label[d][j + (i - 1)*width] != 0 && label[d][(j - 1) + i * this->width] != 0) { // �� �� ������ �� �� ���� ���� ���ؾ� #2
						if (label[d][j + (i - 1)*width] < label[i][j - 1]) {
							zlabel = label[d][j + (i - 1)*width];
							//eqv[label[i][j - 1]] = eqv[label[i - 1][j]]; // equivalency label  ����
							eqv[d][label[d][j - 1 + i * width]] = eqv[d][label[d][j + (i - 1)*width]];
						}
						else {
							zlabel = label[d][(j - 1) + i * this->width];
							//eqv[label[i - 1][j]] = eqv[label[i][j - 1]];
							eqv[d][label[d][j + (i - 1) * width]] = eqv[d][label[d][j - 1 + i * width]];
						}
						label[d][j + i * this->width] = zlabel;

					}
					else if (label[d][j + (i - 1)*width] != 0 && label[d][(j - 1) + i * this->width] == 0) { //#3
						label[d][j + i * this->width] = label[d][j + (i - 1) * this->width];

					}
					else {// #4
						//++currentLabelCount;
						label[d][j + i * this->width] = ++currentLabelCount;
					}



				}
			}
		}
	}

short CCL::find(short a, int depth) { // function to find the minimal equvialent label
		int tempe;
		while (eqv[depth][a] != a) {

			tempe = eqv[depth][a];
			eqv[depth][a] = eqv[depth][tempe];
			a = tempe;

		}
		return  a;

	}

void CCL::secondpass() {

		///////////////////////////2nd �о�  equivalency list while �� ������ linking
		for (int d = 0; d < this->depth; d++) {
			for (int i = 1; i < this->height; i++) { // for every pixels except left end and up end
				for (int j = 1; j < this->width; j++) {
					if (label[d][j + i * this->width] != 0) {
						// process if background		   
						label[d][j + i * this->width] = find(label[d][j + i * this->width], d);
						count[d][label[d][j + i * this->width]]++;
					}
				}
			}
		}
	}
		/* visualize of a part

		for (int i = img1->height()/ 4 - 15; i < img1->height()/ 4 + 15; i++) { // for every pixels except left end and up end
			for (int j = img1->width()/ 4 - 15; j < img1->width()/ 4 + 15; j++) {
				printf_s("%d ", label[i][j]);
			}
			printf_s("\n");
		}
		*/


		// ���⼭ count[] �� sort
void CCL::visualize(){

		vector <int> size(1000); int k = 0;
		for (int d = 0; d < this->depth; d++) {
			for (auto i : sort_indexes(count[d])) { // ����� ū ������� ���̺��� ����
				size[k++] = i;
			}
			k = 0;
			for (int i = 1; i < this->height - 1; i++) { // for every pixels except left end and up end //����� ū ���̺� �ش�Ǵ� �ȼ��� 255 �������� 0 ����  ����ȭ
				for (int j = 1; j < this->width - 1; j++) {
					if (label[d][j+i*this->width] == size[k] || label[d][j+i*this->width] == size[k + 1]) { image[d][j+i*width] = 255; }
					else image[d][j+i*width] = 0;
					//printf_s("%d %d %d\n", image_2dCopy[i][j], i, j);

				}
			}

		}
	}

/*
CCL::~CCL()
{
}
*/