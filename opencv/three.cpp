#include<opencv2/imgcodecs.hpp>///////�����û���//////////
#include<opencv2/highgui.hpp>
#include<opencv2/imgproc.hpp>
#include<iostream>

using namespace cv;
using namespace std;

//////////////////////////���ͼƬ/////////////////////////////
void path05() {
	string path = "D:/opencv����/PVYIHTYZ[AAM)O7]0`4[USF.png";
	Mat img = imread(path);
	Mat imgResize, imgCrop;
	//cout << img.size() << endl;
	resize(img, imgResize, Size(), 1.2, 1.2);///��Size(�޸ĵ�ֵ���޸ĵ�ֵ),���ű��������ű�����


	Rect roi(120, 120, 1, 120);//////////roi(�����Ե���룬�ඥ�˾��룬������);
	imgCrop = img(roi);

	imshow("������С����ʾ", img);//////////////��ȡͼƬ///////////
	//imshow("�ĺ��ͼ��", imgResize);
	imshow("�ü�����ʾ", imgCrop);
	waitKey(0);
}


