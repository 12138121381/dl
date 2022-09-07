#include<opencv2/imgcodecs.hpp>///////�����û���//////////
#include<opencv2/highgui.hpp>
#include<opencv2/imgproc.hpp>
#include<iostream>

using namespace cv;
using namespace std;

//////////////////////////���ͼƬ/////////////////////////////
void path04() {
	string path = "D:/opencv����/PVYIHTYZ[AAM)O7]0`4[USF.png";
	Mat img = imread(path);
	Mat imgGray, imgBlur, imgCanny, imgDil, imgErode;

	cvtColor(img, imgGray, COLOR_BGR2GRAY);/////////��ɫ///////////
	GaussianBlur(img, imgBlur, Size(1, 1), 120, 120);///ģ��/////////Size(�ں˴�С/ģ���̶ȣ���������������)
	Canny(imgBlur, imgCanny, 300, 300);//////���///(����Խ�٣����Խ�ࣩ



	Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
	dilate(imgCanny, imgDil, kernel);///////////�����/////////////
	erode(imgDil, imgErode, kernel);


	//imshow("��ʾ", img);//////////////��ȡͼƬ///////////
	//imshow("ģ��", imgBlur);
	//imshow("���", imgCanny);
	//imshow("�ڰ�", imgGray);
	//imshow("�����", imgDil);
	//imshow("��ʴ(С�����)",imgErode );
	waitKey(0);
}

