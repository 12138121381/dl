#include<iostream>
#include<opencv2/opencv.hpp>
#include<fstream>   //文本读写
#include<Windows.h> //控制台输出中文乱码
#include"successful2.h" //putText显示中文乱码

using namespace std;
using namespace cv;

//自定义车牌结构体
struct License
{
	Mat mat;  //ROI图片
	Rect rect; //ROI所在矩形
};

//获取车牌所在ROI区域--车牌定位
bool Get_License_ROI(Mat src, License& License_ROI)
{
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
					License_ROI = { src(rect),rect };
				}
			}
		}
	}

	if (License_ROI.mat.empty())
	{
		return false;
	}
	return true;
}


//获取车牌每一个字符ROI区域
bool Get_Character_ROI(License& License_ROI, vector<License>& Character_ROI)
{
	Mat gray;
	cvtColor(License_ROI.mat, gray, COLOR_BGR2GRAY);

	Mat thresh;
	threshold(gray, thresh, 0, 255, THRESH_BINARY | THRESH_OTSU);

	Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
	Mat close;
	morphologyEx(thresh, close, MORPH_CLOSE, kernel);

	vector<vector<Point>>contours;
	findContours(close, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
	for (int i = 0; i < contours.size(); i++)
	{
		double area = contourArea(contours[i]);
		//由于我们筛选出来的轮廓是无序的，故后续我们需要将字符重新排序
		if (area > 200)
		{
			Rect rect = boundingRect(contours[i]);
			//计算外接矩形宽高比
			double ratio = double(rect.height) / double(rect.width);
			if (ratio > 1)
			{
				Mat roi = License_ROI.mat(rect);
				resize(roi, roi, Size(50, 100), 1, 1, INTER_LINEAR);
				Character_ROI.push_back({ roi ,rect });
			}
		}
	}

	//将筛选出来的字符轮廓 按照其左上角点坐标从左到右依次顺序排列
	//冒泡排序
	for (int i = 0; i < Character_ROI.size() - 1; i++)
	{
		for (int j = 0; j < Character_ROI.size() - 1 - i; j++)
		{
			if (Character_ROI[j].rect.x > Character_ROI[j + 1].rect.x)
			{
				License temp = Character_ROI[j];
				Character_ROI[j] = Character_ROI[j + 1];
				Character_ROI[j + 1] = temp;
			}
		}
	}

	if (Character_ROI.size() != 7)
	{
		return false;
	}
	return true;
}


//从txt文件中读取中文，防止乱码
string UTF8ToGB(const char* str)
{
	string result;
	WCHAR* strSrc;
	LPSTR szRes;

	//获得临时变量的大小
	int i = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	strSrc = new WCHAR[i + 1];
	MultiByteToWideChar(CP_UTF8, 0, str, -1, strSrc, i);

	//获得临时变量的大小
	i = WideCharToMultiByte(CP_ACP, 0, strSrc, -1, NULL, 0, NULL, NULL);
	szRes = new CHAR[i + 1];
	WideCharToMultiByte(CP_ACP, 0, strSrc, -1, szRes, i, NULL, NULL);

	result = szRes;
	delete[]strSrc;
	delete[]szRes;

	return result;
}

//读取文件  图片
bool Read_Data(string filename, vector<Mat>& dataset)
{
	vector<String>imagePathList;
	glob(filename, imagePathList);
	if (imagePathList.empty())return false;

	for (int i = 0; i < imagePathList.size(); i++)
	{
		Mat image = imread(imagePathList[i]);
		resize(image, image, Size(50, 100), 1, 1, INTER_LINEAR);
		dataset.push_back(image);
	}

	return true;
}

//读取文件 标签
bool Read_Data(string filename, vector<string>& data_name)
{
	fstream fin;
	fin.open(filename, ios::in);
	if (!fin.is_open())
	{
		cout << "can not open the file!" << endl;
		return false;
	}

	string s;
	while (std::getline(fin, s))
	{
		string str = UTF8ToGB(s.c_str()).c_str();
		data_name.push_back(str);
	}
	fin.close();

	return true;
}



//计算像素点个数
int pixCount(Mat image)
{
	int count = 0;
	if (image.channels() == 1)
	{
		for (int i = 0; i < image.rows; i++)
		{
			for (int j = 0; j < image.cols; j++)
			{
				if (image.at<uchar>(i, j) == 0)
				{
					count++;
				}
			}
		}

		return count;
	}
	else
	{
		return -1;
	}
}

//识别车牌字符
bool License_Recognition(vector<License>& Character_ROI, vector<int>& result_index)
{
	string filename = "data/";
	vector<Mat>dataset;
	if (!Read_Data(filename, dataset)) return false;

	for (int i = 0; i < Character_ROI.size(); i++)
	{
		Mat roi_gray;
		cvtColor(Character_ROI[i].mat, roi_gray, COLOR_BGR2GRAY);

		Mat roi_thresh;
		threshold(roi_gray, roi_thresh, 0, 255, THRESH_BINARY | THRESH_OTSU);

		int minCount = 1000000;
		int index = 0;
		for (int j = 0; j < dataset.size(); j++)
		{
			Mat temp_gray;
			cvtColor(dataset[j], temp_gray, COLOR_BGR2GRAY);

			Mat temp_thresh;
			threshold(temp_gray, temp_thresh, 0, 255, THRESH_BINARY | THRESH_OTSU);

			//计算两张图片的像素差，以此判断两张图片是否相同
			Mat dst;
			absdiff(roi_thresh, temp_thresh, dst);

			int count = pixCount(dst);
			if (count < minCount)
			{
				minCount = count;
				index = j;
			}
		}

		result_index.push_back(index);
	}

	return true;
}

//显示最终效果
bool Draw_Result(Mat src, License& License_ROI, vector<License>& Character_ROI, vector<int>& result_index)
{
	rectangle(src, License_ROI.rect, Scalar(0, 255, 0), 2);

	vector<string>data_name;
	if (!Read_Data("data_name.txt", data_name))return false;

	for (int i = 0; i < Character_ROI.size(); i++)
	{
		cout << data_name[result_index[i]] << " ";
		//putText 中文显示会乱码，所以采用下面代码
		CvxText text("C://Windows/Fonts/方正粗黑宋简体.ttf");//字体
		string str = data_name[result_index[i]];  //string 转 char
		const char* msg = str.data();
		IplImage* temp;  //Mat 转 IplImage
		temp = &IplImage(src);
		text.putText(temp, msg, Point(License_ROI.rect.x + Character_ROI[i].rect.x, License_ROI.rect.y + Character_ROI[i].rect.y), Scalar(0, 0, 255));
	}
	return true;
}


int main()
{

	Mat src = imread("car.jpg");
	if (src.empty())
	{
		cout << "No image!" << endl;
		system("pause");
		return -1;
	}

	License License_ROI;
	if (Get_License_ROI(src, License_ROI))
	{
		vector<License>Character_ROI;
		if (Get_Character_ROI(License_ROI, Character_ROI))
		{
			vector<int>result_index;
			if (License_Recognition(Character_ROI, result_index))
			{
				Draw_Result(src, License_ROI, Character_ROI, result_index);
			}
			else
			{
				cout << "未能识别字符！" << endl;
				system("pause");
				return -1;
			}
		}
		else
		{
			cout << "未能切割出字符！" << endl;
			system("pause");
			return -1;
		}
	}
	else
	{
		cout << "未定位到车牌位置！" << endl;
		system("pause");
		return -1;
	}

	imshow("src", src);
	waitKey(0);
	system("pause");
	return 0;
}