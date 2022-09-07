#include<opencv2/imgcodecs.hpp>///////数字用基数//////////
#include<opencv2/highgui.hpp>
#include<opencv2/imgproc.hpp>
#include<iostream>

using namespace cv;
using namespace std;

//////////////////////////添加图片/////////////////////////////
void path04() {
	string path = "D:/opencv工具/PVYIHTYZ[AAM)O7]0`4[USF.png";
	Mat img = imread(path);
	Mat imgGray, imgBlur, imgCanny, imgDil, imgErode;

	cvtColor(img, imgGray, COLOR_BGR2GRAY);/////////灰色///////////
	GaussianBlur(img, imgBlur, Size(1, 1), 120, 120);///模糊/////////Size(内核大小/模糊程度（横拉长，纵拉长)
	Canny(imgBlur, imgCanny, 300, 300);//////描边///(数字越少，描边越多）



	Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
	dilate(imgCanny, imgDil, kernel);///////////厚描边/////////////
	erode(imgDil, imgErode, kernel);


	//imshow("显示", img);//////////////读取图片///////////
	//imshow("模糊", imgBlur);
	//imshow("描边", imgCanny);
	//imshow("黑白", imgGray);
	//imshow("厚描边", imgDil);
	//imshow("侵蚀(小厚描边)",imgErode );
	waitKey(0);
}

