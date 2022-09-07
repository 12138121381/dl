#include<opencv2/imgcodecs.hpp>          //img=src//
#include<opencv2/highgui.hpp>
#include<opencv2/imgproc.hpp>
#include<iostream>

using namespace cv;
using namespace std;


//自定义车牌结构体
struct License
{
	Mat mat;  //ROI图片
	Rect rect; //ROI所在矩形
};


////////////////////////添加图片/////////////////////////////
void path01() {
	string path = "C:/Users/86157/Desktop/opencv练习/ZT88I0][1}4Q6D3UOF_R70V.png";
	Mat src = imread(path);

	Mat gray;
	cvtColor(src, gray, COLOR_BGR2GRAY);

	Mat thresh;
	threshold(gray, thresh, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);

	//使用形态学开操作去除一些小轮廓
	Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
	Mat open;
	morphologyEx(thresh, open, MORPH_OPEN, kernel);


	//使用 RETR_EXTERNAL 找到最外轮廓
	vector<vector<Point>>contours;
	findContours(open, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
	vector<vector<Point>>conPoly(contours.size());
	for (int i = 0; i < contours.size(); i++)
	{
		double area = contourArea(contours[i]);
		double peri = arcLength(contours[i], true);
		//根据面积筛选出可能属于车牌区域的轮廓
		if (area > 1000)
		{
			//使用多边形近似，进一步确定车牌区域轮廓
			approxPolyDP(contours[i], conPoly[i], 0.02 * peri, true);

			if (conPoly[i].size() == 4)
			{
				//计算矩形区域宽高比
				Rect box = boundingRect(contours[i]);
				double ratio = double(box.width) / double(box.height);
				if (ratio > 2 && ratio < 4)
				{
					//截取ROI区域
					Rect rect = boundingRect(contours[i]);
					Mat mat = { src(rect),rect };
				}
			}
		}
	}


	imshow("image", contours);
	waitKey(0);
}



void main() {

	path01();
}
