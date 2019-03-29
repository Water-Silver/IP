#include <vector>

using namespace std;
class CCL
{

public:
	CCL() {}
	~CCL()
	{}
	int depth;
	int width;
	int height;
	short currentLabelCount = 1;
	short **label;
	short *eqv;
	vector < vector<short> >count;
	short **image;

	void InitMembers(short ** image, int d, int w, int h);
	void firstpass();
	short find(short a);
	short find(short a, int depth);
	void secondpass();
	void visualize();

};
