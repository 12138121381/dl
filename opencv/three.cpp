#include<opencv2/imgcodecs.hpp>///////数字用基数//////////
#include<opencv2/highgui.hpp>
#include<opencv2/imgproc.hpp>
#include<iostream>

using namespace cv;
using namespace std;

//////////////////////////添加图片/////////////////////////////
void path05() {
	string path = "D:/opencv工具/PVYIHTYZ[AAM)O7]0`4[USF.png";
	Mat img = imread(path);
	Mat imgResize, imgCrop;
	//cout << img.size() << endl;
	resize(img, imgResize, Size(), 1.2, 1.2);///（Size(修改的值，修改的值),缩放比例，缩放比例）


	Rect roi(120, 120, 1, 120);//////////roi(距左边缘距离，距顶端距离，长，宽);
	imgCrop = img(roi);

	imshow("调整大小与显示", img);//////////////读取图片///////////
	//imshow("改后的图形", imgResize);
	imshow("裁剪与显示", imgCrop);
	waitKey(0);
}


