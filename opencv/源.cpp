#include<opencv2/imgcodecs.hpp>          //img=src//
#include<opencv2/highgui.hpp>
#include<opencv2/imgproc.hpp>
#include<iostream>

using namespace cv;
using namespace std;


//�Զ��峵�ƽṹ��
struct License
{
	Mat mat;  //ROIͼƬ
	Rect rect; //ROI���ھ���
};


////////////////////////���ͼƬ/////////////////////////////
void path01() {
	string path = "C:/Users/86157/Desktop/opencv��ϰ/ZT88I0][1}4Q6D3UOF_R70V.png";
	Mat src = imread(path);

	Mat gray;
	cvtColor(src, gray, COLOR_BGR2GRAY);

	Mat thresh;
	threshold(gray, thresh, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);

	//ʹ����̬ѧ������ȥ��һЩС����
	Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
	Mat open;
	morphologyEx(thresh, open, MORPH_OPEN, kernel);


	//ʹ�� RETR_EXTERNAL �ҵ���������
	vector<vector<Point>>contours;
	findContours(open, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
	vector<vector<Point>>conPoly(contours.size());
	for (int i = 0; i < contours.size(); i++)
	{
		double area = contourArea(contours[i]);
		double peri = arcLength(contours[i], true);
		//�������ɸѡ���������ڳ������������
		if (area > 1000)
		{
			//ʹ�ö���ν��ƣ���һ��ȷ��������������
			approxPolyDP(contours[i], conPoly[i], 0.02 * peri, true);

			if (conPoly[i].size() == 4)
			{
				//������������߱�
				Rect box = boundingRect(contours[i]);
				double ratio = double(box.width) / double(box.height);
				if (ratio > 2 && ratio < 4)
				{
					//��ȡROI����
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
