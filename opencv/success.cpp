#include <opencv2/opencv.hpp>
#include<opencv2/imgproc/imgproc.hpp>
#include<opencv2/highgui/highgui.hpp>
#include"Globa.h"
#include <windows.h>
#include <string>

using namespace std;
using namespace cv;

void fillHole(const Mat srcBw, Mat &dstBw);           //填补算法
Mat cutOne(Mat cutImage);         //边框切割算法
void CharCut(Mat srcImage);            //单个字符切割算法
Mat Location(Mat srcImage);            //图像识别算法
void SingleCharCut(Mat doubleImage, int k1, int k2);
void ShowChar();
void MatchProvince();
void MatchNumber();
void readProvince();
void readNumber();
void VideoShow(Mat videoImage);
void GetStringSize(HDC hDC, const char* str, int* w, int* h);
void putTextZH(Mat &dst, const char* str, Point org, Scalar color, int fontSize, const char* fn, bool italic, bool underline);




int flag_1;          //判断是否倾斜，需不需要二次定位车牌
bool flag;       //判断提取是否成功
bool specialFlag = false;    //针对嵌套车牌
int captureRead = 0;
int videoFlag = 0;
string carPlateProvince = " ";
string carPlate = " ";
char test[10];
vector<Mat>  singleChar;         //字符图片容器

int main(int argc, char *argv[])
{
	//计时开始
	double time0 = static_cast<double>(getTickCount());

	//视频操作
	VideoCapture capture("1.mp4");
	Mat srcImage;
	Mat theFirst;
	int singleCharLength;

	//读取字符图片
	readProvince();
	readNumber();

	while (1) {
		capture >> srcImage;
		try {

			if (!srcImage.data) { printf("视频识别结束    \n"); return 0; }

			if (srcImage.rows >= srcImage.cols)
			{
				resize(srcImage, srcImage, Size(640, 640 * srcImage.rows / srcImage.cols));
			}
			else
			{
				resize(srcImage, srcImage, Size(400 * srcImage.cols / srcImage.rows, 400));
			}

			//车牌定位

			theFirst = Location(srcImage);

			if (flag)
			{
				if (flag_1 == 1)                      //旋转后要再次定位去上下杂边
				{
					theFirst = Location(theFirst);
					flag_1 = 0;
				}
			}
			if (flag)
			{
				//车牌切割(切割上下边，去除干扰)
				theFirst = cutOne(theFirst);
				//单个字符切割
				CharCut(theFirst);
				singleCharLength = singleChar.size();
				printf("采取字符轮廓数                               %d\n", singleCharLength);
				ShowChar();
				if (singleCharLength >= 7)
				{

					MatchProvince();
					MatchNumber();
				}

				singleChar.clear();
			}

		}
		catch (Exception e) {

			cout << "Standard ecxeption : " << e.what() << " \n" << endl;

		}

		if (waitKey(30) >= 0)
			break;
		//延时30ms

	}
		//imwrite("match\\xxxxxx.bmp", singleChar[2]);


	                
	time0 = ((double)getTickCount() - time0) / getTickFrequency();
	cout << "运行时间" << time0 << "秒" << endl;


	waitKey(0);
}

void fillHole(const Mat srcBw, Mat &dstBw)
{
	Size imageSize = srcBw.size();
	Mat Temp = Mat::zeros(imageSize.height + 2, imageSize.width + 2, srcBw.type());//延展图像
	srcBw.copyTo(Temp(Range(1, imageSize.height + 1), Range(1, imageSize.width + 1)));

	cv::floodFill(Temp, Point(0, 0), Scalar(255));

	Mat cutImg;//裁剪延展的图像
	Temp(Range(1, imageSize.height + 1), Range(1, imageSize.width + 1)).copyTo(cutImg);

	dstBw = srcBw | (~cutImg);
}

Mat Location(Mat srcImage)
{
	//判断变量重赋值
	flag = false;

	//用于旋转车牌
	int	imageWidth, imageHeight;            //输入图像的长和宽
	imageWidth = srcImage.rows;                 //获取图片的宽
	imageHeight = srcImage.cols;                 //获取图像的长
	//!!!!!!!!!!!!!!!!!!!
	Mat blueROI = srcImage.clone();
	cvtColor(blueROI, blueROI, CV_BGR2HSV);
	//namedWindow("hsv图");
	//imshow("hsv图", blueROI);
	//中值滤波操作
	medianBlur(blueROI, blueROI, 3);
	//namedWindow("medianBlur图");
	//imshow("medianBlur图", blueROI);
	//将蓝色区域二值化
	inRange(blueROI, Scalar(100, 130, 50), Scalar(124, 255, 255), blueROI);
	//namedWindow("blue图");
	//imshow("blue图", blueROI);

	Mat element1 = getStructuringElement(MORPH_RECT, Size(2, 2));     //size（）对速度有影响
	morphologyEx(blueROI, blueROI, MORPH_OPEN, element1);
	//namedWindow("0次K运算后图像");
	//imshow("0次K运算后图像", blueROI);

	Mat element0 = getStructuringElement(MORPH_ELLIPSE, Size(10, 10));     //size（）对速度有影响
	morphologyEx(blueROI, blueROI, MORPH_CLOSE, element0);
	//namedWindow("0次闭运算后图像");
	//imshow("0次闭运算后图像", blueROI);
	vector<vector<Point>> contours;

	findContours(blueROI, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);


	int cnt = contours.size();
	cout << "number of contours   " << cnt << endl;  //打印轮廓个数
	if (cnt == 0)
	{
		if (!flag)        //在视频中显示
		{
			cout << "图中无车牌       " << endl;
			//namedWindow("提取车牌结果图");
			//imshow("提取车牌结果图", srcImage);    //显示最终结果图
			VideoShow(srcImage);
			return  srcImage;
		}
	}



	double area;
	double longside, temp, shortside, long2short;
	float  angle = 0;
	Rect rect;
	RotatedRect box;    //可旋转的矩形盒子
	Point2f vertex[4];        //四个顶点

	Mat image = srcImage.clone();        //为后来显示做准备
	Mat  rgbCutImg;                       //车牌裁剪图

	//box.points(vertex);            //获取矩形四个顶点坐标
	//length=arcLength(contour[i]);                        //获取轮廓周长
	//area=contourArea(contour[i]);                        //获取轮廓面积
	//angle=box.angle;           //得到车牌倾斜角度

	for (int i = 0; i < cnt; i++)
	{
		area = contourArea(contours[i]);              //获取轮廓面积
		if (area > 600 && area < 15000)     //矩形区域面积大小判断
		{
			rect = boundingRect(contours[i]);    //计算矩形边界
			box = minAreaRect(contours[i]);      //获取轮廓的矩形
			box.points(vertex);                  //获取矩形四个顶点坐标
			angle = box.angle;                   //得到车牌倾斜角度

			longside = sqrt(pow(vertex[1].x - vertex[0].x, 2) + pow(vertex[1].y - vertex[0].y, 2));
			shortside = sqrt(pow(vertex[2].x - vertex[1].x, 2) + pow(vertex[2].y - vertex[1].y, 2));
			if (shortside > longside)   //短轴大于长轴，交换数据
			{
				temp = longside;
				longside = shortside;
				shortside = temp;
				cout << "交换" << endl;
			}
			else
				angle += 90;
			long2short = longside / shortside;
			if (long2short > 1.5 && long2short < 4.5)
			{
				flag = true;
				for (int i = 0; i < 4; ++i)       //划线框出车牌区域
					line(image, vertex[i], vertex[((i + 1) % 4) ? (i + 1) : 0], Scalar(0, 255, 0), 1, CV_AA);


				if (!flag_1)        //在视频中显示
				{
					printf("提取成功\n");
					/*namedWindow("提取车牌结果图");
					imshow("提取车牌结果图", image);  */  //显示最终结果图
					VideoShow(image);
				}

				rgbCutImg = srcImage(rect);
				//namedWindow("车牌图");
				//imshow("车牌图", rgbCutImg);//裁剪出车牌	
				break;              //退出循环，以免容器中变量变换
			}
		}
	}
	cout << "倾斜角度：" << angle << endl;
	if (flag  &&  fabs(angle) > 0.8)        //车牌过偏，转一下                偏移角度小时可不调用，后续找到合适范围再改进
	{
		flag_1 = 1;
		Mat RotractImg(imageWidth, imageHeight, CV_8UC1, Scalar(0, 0, 0));       //倾斜矫正图片
		Point2f center = box.center;           //获取车牌中心坐标
		Mat M2 = getRotationMatrix2D(center, angle, 1);       //计算旋转加缩放的变换矩阵 
		warpAffine(srcImage, RotractImg, M2, srcImage.size(), 1, 0, Scalar(0));       //进行倾斜矫正
		//namedWindow("倾斜矫正后图片",0);
		//imshow("倾斜矫正后图片", RotractImg);
		rgbCutImg = RotractImg(rect);      //截取车牌彩色照片
		//namedWindow("矫正后车牌照");
		//imshow("矫正后车牌照", rgbCutImg);
			/*cout << "矩形中心:" << box.center.x << "," << box.center.y << endl;*/
		return  rgbCutImg;
	}

	if (flag == false) {
		printf("提取失败\n");                      //后期加边缘检测法识别
		if (!flag_1)        //在视频中显示
		{
			/*namedWindow("提取车牌结果图");
			imshow("提取车牌结果图", image); */   //显示最终结果图
			VideoShow(image);
		}
	}
	return rgbCutImg;
}

Mat cutOne(Mat cutImage)
{
	//打印车牌长宽
	try {
		/*cout << "           cutImage.rows  :   " << cutImage.rows << endl;
		cout << "           cutImage.cols  :   " << cutImage.cols << endl;*/
		if(cutImage.rows >= cutImage.cols)
		resize(cutImage, cutImage, Size(320, 320 * cutImage.rows / cutImage.cols));
	}
	catch (Exception e)
	{
		resize(cutImage, cutImage, Size(320, 100));
	}
	/*namedWindow("Resize车牌图");
	imshow("Resize车牌图", cutImage);*/
	int height = cutImage.rows;
	cout << "\tHeight:" << height << "\tWidth:" << 320 << endl;
	if (height < 86)
	{
		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!处理新型嵌套车牌!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		printf("嵌套车牌\n");
		specialFlag = true;

	}

	Mat whiteROI = cutImage.clone();

	if (specialFlag)
	{
		cvtColor(whiteROI, whiteROI, CV_BGR2HSV);
		//将白色区域二值化
		//inRange(whiteROI, Scalar(0, 0, 0), Scalar(130, 50, 245), whiteROI);      //增大 S 即饱和度可以使hsv白色检测范围更大
		inRange(whiteROI, Scalar(0, 0, 0), Scalar(180, 100, 245), whiteROI);
		//namedWindow("specialFlagwhiteROI图");
		//imshow("specialFlagwhiteROI图", whiteROI);
	}
	else
	{
		GaussianBlur(whiteROI, whiteROI, Size(3, 3), 0, 0);
		/*namedWindow("GaussianBlur车牌图");
		imshow("GaussianBlur车牌图", whiteROI);*/

		cvtColor(whiteROI, whiteROI, CV_BGR2HSV);

		//medianBlur(whiteROI, whiteROI, 3);
		//namedWindow("Src_medianBlur图");
		//imshow("Src_medianBlur图", whiteROI);

		//将白色区域二值化
		//inRange(whiteROI, Scalar(0, 0, 10), Scalar(180, 30, 255), whiteROI);      //增大 S 即饱和度可以使hsv白色检测范围更大
		inRange(whiteROI, Scalar(0, 0, 10), Scalar(180, 120, 255), whiteROI);
		//namedWindow("whiteROI图");
		//imshow("whiteROI图", whiteROI);
	}


	/*
	Mat element0 = getStructuringElement(MORPH_ELLIPSE, Size(4, 4));     //size（）对速度有影响
	morphologyEx(whiteROI, whiteROI, MORPH_OPEN, element0);
	namedWindow("OPEN图");
	imshow("OPEN图", whiteROI);
	*/
	Mat dstImage = cutImage.clone();
	vector<vector<Point>> contours;
	findContours(whiteROI, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
	drawContours(dstImage, contours, -1, Scalar(0, 0, 255), 1);
	//namedWindow("疑似字符轮廓识别图");
	//imshow("疑似字符轮廓识别图", dstImage);
	inRange(dstImage, Scalar(0, 0, 255), Scalar(0, 0, 255), dstImage);
	//namedWindow("字符大轮廓图");
	//imshow("字符大轮廓图", dstImage);
	/*fillHole(dstImage, dstImage);
	namedWindow("填补轮廓后图");
	imshow("填补轮廓后图", dstImage);*/

	int row1 = 2;
	int row2 = dstImage.rows;
	int rowMax = dstImage.rows - 1;            //开区间，防止越界
	int colMax = dstImage.cols - 1;            //开区间，防止越界
	int addFirst = 10;
	int addFirst0 = 0;
	int addFirst1 = 0;
	int addFirst2 = 0;
	//测中间像素
	//dstImage.at<uchar>(rowMax-1, colMax-1);
	//cout << "Width:" << j << endl;

	int addFirstTemp = addFirst;          //第一次用时已经改变数值，容易忽略！！！！！

	uchar* data;

	//裁剪上下边。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。
	//上边
	for (int i = 2; i < rowMax / 3; i++, addFirst1 = 0)                           //   6     刚刚好
	{
		data = dstImage.ptr<uchar>(i);
		for (int j = 2; j < colMax; j++)
		{
			if (data[j] == 255)
			{
				addFirst1++;
			}
		}
		if (addFirst1 < addFirst)                       //筛选最小值所在行
		{
			row1 = i;
			addFirst = addFirst1 + 3;
			//cout << "行头" << row1 << endl;
			//flag_x = 1;
		}
	}
	//下边
	for (int i = rowMax - 2; i > rowMax - rowMax / 4; i--, addFirst2 = 0)                //   6     刚刚好
	{
		data = dstImage.ptr<uchar>(i);
		for (int j = 2; j < colMax; j++)
		{
			if (data[j] == 255)
			{
				addFirst2++;
			}
		}
		if (addFirst2 < addFirstTemp)                       //筛选最小值所在行
		{
			row2 = i;
			addFirstTemp = addFirst2 + 3;
			//cout << "行底" << row2 << endl;
			//flag_y = 1;
		}
	}


	int orow;
	orow = row2 - row1;
	Mat w_image;
	Mat  rgb_w_image;
	w_image = dstImage(Rect(0, row1, colMax, orow));
	rgb_w_image = cutImage(Rect(0, row1, colMax, orow));
	//namedWindow("裁剪上下图");
	//imshow("裁剪上下图", w_image);

	int rowMax_ALT = w_image.rows - 1;            //开区间，防止越界(注意，裁剪完上下后要重新写行和宽，因为行和宽已经改变)
	int colMax_ALT = w_image.cols - 1;            //开区间，防止越界（注意，裁剪完上下后要重新写行和宽，因为行和宽已经改变）
	int col_1 = 2;
	int col_2 = w_image.cols;
	int add = 2;
	int add1 = 0;
	int add2 = 0;

	int addTemp = add;        //第一次用时已经改变数值，容易忽略！！！！！

	//裁剪左右边。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。
	//左边
	//for (int i = 0; i < colMax_ALT / 18; i++, add1 = 0)                           //       刚刚好
	//{
	//	for (int j = 2; j < rowMax_ALT; j++)
	//	{
	//		data = dstImage.ptr<uchar>(j);
	//		if (data[i] == 255)
	//		{
	//			add1++;
	//		}
	//	}
	//	if (add1 < add)                       //筛选最小值所在列
	//	{
	//		col_1 = i;
	//		add = add1 + 1;
	//	}
	//}
	//右边
	if (specialFlag)
	{
		for (int i = colMax_ALT; i > colMax_ALT - colMax_ALT / 18; i--, add2 = 0)                //        刚刚好
		{
			for (int j = 2; j < rowMax_ALT; j++)
			{
				data = dstImage.ptr<uchar>(j);
				if (data[i] == 255)
				{
					add2++;
				}
			}
			if (add2 < addTemp)                       //筛选最小值所在列
			{
				col_2 = i;
				addTemp = add2 + 1;
				//cout << "行底" << row2 << endl;
			}
		}
	}
	int o_col;
	o_col = col_2 - col_1;

	Mat H_image;
	H_image = w_image(Rect(col_1, 0, o_col, rowMax_ALT));
	rgb_w_image = rgb_w_image(Rect(col_1, 0, o_col, rowMax_ALT));
	//namedWindow("再裁剪左右图");
	//imshow("再裁剪左右图", H_image);
	//namedWindow("裁剪后彩图");
	//imshow("裁剪后彩图", rgb_w_image);

	return rgb_w_image;
}

void CharCut(Mat srcImage)
{
	resize(srcImage, srcImage, Size(320, 320 * srcImage.rows / srcImage.cols));
	//namedWindow("Resize车牌图");
	//imshow("Resize车牌图", srcImage);
	GaussianBlur(srcImage, srcImage, Size(3, 3), 0, 0);
	/*namedWindow("GaussianBlur车牌图");
	imshow("GaussianBlur车牌图", srcImage);
*/
	medianBlur(srcImage, srcImage, 3);
	//namedWindow("Src_medianBlur图");
	//imshow("Src_medianBlur图", srcImage);

	cvtColor(srcImage, srcImage, CV_BGR2HSV);


	//将白色区域二值化
	Mat doubleImage;
	//inRange(srcImage, Scalar(0, 0, 10), Scalar(180, 75, 255), doubleImage);      //增大 S 即饱和度可以使hsv白色检测范围更大
	inRange(srcImage, Scalar(0, 0, 0), Scalar(180, 125, 245), doubleImage);
	namedWindow("doubleImage图");
	imshow("doubleImage图", doubleImage);

	int colTemp = 0;
	int rowMax = doubleImage.rows;
	int colMax = doubleImage.cols;
	int addFirst = 0;
	int add = 0;
	int k1 = 0;
	int k2;
	int kTemp = 0;
	int times = 0;
	int oneCutEnd = 0;
	float t = 1.0;
	uchar* data;

	cout << "Test:            " << specialFlag << endl;


	//针对嵌套车牌处理
	if (specialFlag)
	{
		for (int i = 2; i < colMax; i++, addFirst = 0, add = 0)
		{
			for (int j = rowMax / 10.8; j < rowMax - rowMax / (10.8*t); j++)
			{
				data = doubleImage.ptr<uchar>(j);
				if (data[i - 1] == 255)
				{
					addFirst++;                             //统计前一列
				}
			}

			for (int j = rowMax / 10.8; j < rowMax - rowMax / (10.8*t); j++)
			{
				data = doubleImage.ptr<uchar>(j);
				if (data[i] == 255)
				{
					add++;                                  //统计后一列
				}
			}
			//省份字符分开切割
			if (!times)
			{
				if (!oneCutEnd && (!addFirst  &&  add))
					k1 = i - 1;
				if (addFirst && !add)
				{
					k2 = i;
					oneCutEnd = 1;
					if (k2 - k1 > colMax / 11.25)
					{
						times = 1;
						if (k2 - k1 < colMax / 5.625)
							SingleCharCut(doubleImage, k1, k2);
						else
							i = 2;
					}
				}
			}         //切割其他字符
			else {

				if (!addFirst  &&  add)
					k1 = i - 1;
				if (addFirst && !add)
				{
					k2 = i;
					if (k2 - k1 > colMax / 32)
					{
						if (k2 - k1 < colMax / 5.625)
							SingleCharCut(doubleImage, k1, k2);
						else                                   //针对嵌套车牌下部连接过靠上
						{
							i = k1;
							t -= 0.1;
						}
					}
					else
					{   //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!处理中间分割点与‘ 1 ’!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
						for (int a = k1; a <= k2; a++)
						{
							data = doubleImage.ptr<uchar>(rowMax / 5);
							if (data[a] == 255)
								kTemp++;
						}
						if (kTemp > 0)
							SingleCharCut(doubleImage, k1, k2);
						kTemp = 0;
					}
				}

			}

		}

		k2 = colMax;
		if (k2 - k1 > colMax / 32)
			SingleCharCut(doubleImage, k1, k2);

		specialFlag = false;

	}
	else {
		for (int i = 2; i < colMax; i++, addFirst = 0, add = 0)
		{
			for (int j = rowMax / 12.8; j < rowMax - rowMax / 12.8; j++)
			{
				data = doubleImage.ptr<uchar>(j);
				if (data[i - 1] == 255)
				{
					addFirst++;
				}
			}

			for (int j = rowMax / 12.8; j < rowMax - rowMax / 12.8; j++)
			{
				data = doubleImage.ptr<uchar>(j);
				if (data[i] == 255)
				{
					add++;
				}
			}


			if (!times)
			{
				if (!oneCutEnd && (!addFirst  &&  add))
					k1 = i - 1;
				if (addFirst && !add)
				{
					k2 = i;
					oneCutEnd = 1;
					if (k2 - k1 > colMax / 11.25)
					{
						times = 1;
						if (k2 - k1 < colMax / 5.625)
							SingleCharCut(doubleImage, k1, k2);
						else
							i = 2;
					}
				}
			}
			else {

				if (!addFirst  &&  add)
					k1 = i - 1;
				if (addFirst && !add)
				{
					k2 = i;
					if (k2 - k1 > colMax / 32)
						SingleCharCut(doubleImage, k1, k2);
					else
					{   //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!处理中间分割点与‘ 1 ’!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
						for (int a = k1; a <= k2; a++)
						{
							data = doubleImage.ptr<uchar>(rowMax / 5);
							if (data[a] == 255)
								kTemp++;
						}
						if (kTemp > 0)
							SingleCharCut(doubleImage, k1, k2);
						kTemp = 0;
					}
				}

			}







		}
	}
}

void SingleCharCut(Mat doubleImage, int k1, int k2)
{
	//printf("k1 = %d ,k2 = %d\n", k1, k2);


	int rowMax = doubleImage.rows;
	Mat image;
	image = doubleImage(Rect(k1, 0, k2 - k1, rowMax));

	int row1 = 0;
	int row2 = image.rows;
	rowMax = image.rows - 1;            //开区间，防止越界
	int colMax = image.cols;            //开区间，防止越界
	int addFirst = 2;
	int addFirst1 = 0;
	int addFirst2 = 0;
	uchar* data;
	//测中间像素
	//image.at<uchar>(rowMax-1, colMax-1);
	//cout << "Width:" << j << endl;

	int addFirstTemp = addFirst;          //第一次用时已经改变数值，容易忽略！！！！！

	//裁剪上下边。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。。
	//上边
	for (int i = 0; i < rowMax / 4; i++, addFirst1 = 0)                           //   6     刚刚好
	{
		data = image.ptr<uchar>(i);
		for (int j = 0; j < colMax; j++)
		{
			if (data[j] == 255)
			{
				addFirst1++;
			}
		}
		if (addFirst1 < addFirst)                       //筛选最小值所在行
		{
			row1 = i;
			addFirst = addFirst1 + 1;
		}
	}

	//下边
	for (int i = rowMax; i > rowMax - rowMax / 4; i--, addFirst2 = 0)                //   6     刚刚好
	{
		data = image.ptr<uchar>(i);
		for (int j = 2; j < colMax; j++)
		{
			if (data[j] == 255)
			{
				addFirst2++;
			}
		}
		if (addFirst2 < addFirstTemp)                       //筛选最小值所在行
		{
			row2 = i;
			addFirstTemp = addFirst2 + 1;
		}
	}

	int orow;
	orow = row2 - row1;
	Mat w_image;
	w_image = image(Rect(0, row1, colMax, orow));
	singleChar.push_back(w_image);

}

void ShowChar()
{
	int length = singleChar.size();
	for (int i = 0; i < length; i++) {

		resize(singleChar[i], singleChar[i], Size(20, 40));           //字符图像归一化

		//namedWindow(to_string(i) + "图");
		//imshow(to_string(i) + "图", singleChar[i]);
	}
}




//读取省份模板
struct  stu
{
	Mat image;
	double matchDegree;
};
struct  stu first[35];
void readProvince()
{
	int i = 0;
	//读取字符
	{
		first[i].image = imread("match\\zw1.bmp", 0);
		i++;
		first[i].image = imread("match\\zw2.bmp", 0);
		i++;
		first[i].image = imread("match\\zw3.bmp", 0);
		i++;
		first[i].image = imread("match\\zw4.bmp", 0);
		i++;
		first[i].image = imread("match\\zw5.bmp", 0);
		i++;
		first[i].image = imread("match\\zw6.bmp", 0);
		i++;
		first[i].image = imread("match\\zw7.bmp", 0);
		i++;
		first[i].image = imread("match\\zw8.bmp", 0);
		i++;
		first[i].image = imread("match\\zw9.bmp", 0);
		i++;
		first[i].image = imread("match\\zw10.bmp", 0);
		i++;
		first[i].image = imread("match\\zw11.bmp", 0);
		i++;
		first[i].image = imread("match\\zw12.bmp", 0);
		i++;
		first[i].image = imread("match\\zw13.bmp", 0);
		i++;
		first[i].image = imread("match\\zw14.bmp", 0);
		i++;
		first[i].image = imread("match\\zw15.bmp", 0);
		i++;
		first[i].image = imread("match\\zw16.bmp", 0);
		i++;
		first[i].image = imread("match\\zw17.bmp", 0);
		i++;
		first[i].image = imread("match\\zw18.bmp", 0);
		i++;
		first[i].image = imread("match\\zw19.bmp", 0);
		i++;
		first[i].image = imread("match\\zw20.bmp", 0);
		i++;
		first[i].image = imread("match\\zw21.bmp", 0);
		i++;
		first[i].image = imread("match\\zw22.bmp", 0);
		i++;
		first[i].image = imread("match\\zw23.bmp", 0);
		i++;
		first[i].image = imread("match\\zw24.bmp", 0);
		i++;
		first[i].image = imread("match\\zw25.bmp", 0);
		i++;
		first[i].image = imread("match\\zw26.bmp", 0);
		i++;
		first[i].image = imread("match\\zw27.bmp", 0);
		i++;
		first[i].image = imread("match\\zw28.bmp", 0);
		i++;
		first[i].image = imread("match\\zw29.bmp", 0);
		i++;
		first[i].image = imread("match\\zw30.bmp", 0);
		i++;
		first[i].image = imread("match\\zw31.bmp", 0);
		i++;
		first[i].image = imread("match\\zw32.bmp", 0);
		i++;
		first[i].image = imread("match\\zw33.bmp", 0);
		i++;
		first[i].image = imread("match\\zw34.bmp", 0);
		i++;
		first[i].image = imread("match\\zw35.bmp", 0);
	}


}


//识别省份字符
void MatchProvince()
{
	int rowMax = 40;
	int colMax = 20;
	int add = 0;
	int addTemp = 0;
	Mat absCutImage;
	double temp;
	int index = 0;
	uchar* data;

	for (int i = 0; i < rowMax; i++)
	{
		data = singleChar[0].ptr<uchar>(i);
		for (int j = 0; j < colMax; j++)
		{
			if (data[j] == 255)
			{
				add++;
			}
		}
	}

	int firstLength = end(first) - begin(first);
	//printf("数组长度1         %d\n",firstLength);
	for (int x = 0; x < firstLength; x++, addTemp = 0)
	{
		absCutImage = abs(first[x].image - singleChar[0]);

		for (int i = 0; i < rowMax; i++)
		{
			data = absCutImage.ptr<uchar>(i);
			for (int j = 0; j < colMax; j++)
			{
				if (data[j] == 255)
				{
					addTemp++;
				}
			}
		}
		temp = 1.0 - 1.0*addTemp / add;
		if (temp <= 1)
			first[x].matchDegree = temp;
		else
			first[x].matchDegree = 0;


		if (x > 0 && first[x].matchDegree > first[index].matchDegree)
			index = x;
	}

	/*absCutImage = abs(singleChar[0] - first[index].image);
	namedWindow("省份图片相减后图" + to_string(0));
	imshow("省份图片相减后图" + to_string(0), absCutImage);*/

	printf("省份字符最大匹配度：  %lf\n", first[index].matchDegree);

	switch (index) {
	case 0:
		printf("藏");
		carPlateProvince += "藏";
		break;
	case 1:
		printf("川");
		carPlateProvince += "川";
		break;
	case 2:
		printf("鄂");
		carPlateProvince += "鄂";
		break;
	case 3:
		printf("甘");
		carPlateProvince += "甘";
		break;
	case 4:
		printf("赣");
		carPlateProvince += "赣";
		break;
	case 5:
		printf("贵");
		carPlateProvince += "贵";
		break;
	case 6:
		printf("桂");
		carPlateProvince += "桂";
		break;
	case 7:
		printf("黑");
		carPlateProvince += "黑";
		break;
	case 8:
		printf("泸");
		carPlateProvince += "泸";
		break;
	case 9:
		printf("吉");
		carPlateProvince += "吉";
		break;
	case 10:
		printf("翼");
		carPlateProvince += "翼";
		break;
	case 11:
		printf("津");
		carPlateProvince += "津";
		break;
	case 12:
		printf("晋");
		carPlateProvince += "晋";
		break;
	case 13:
		printf("京");
		carPlateProvince += "京";
		break;
	case 14:
		printf("辽");
		carPlateProvince += "辽";
		break;
	case 15:
		printf("鲁");
		carPlateProvince += "鲁";
		break;
	case 16:
		printf("蒙");
		carPlateProvince += "蒙";
		break;
	case 17:
		printf("闽");
		carPlateProvince += "闽";
		break;
	case 18:
		printf("宁");
		carPlateProvince += "宁";
		break;
	case 19:
		printf("青");
		carPlateProvince += "青";
		break;
	case 20:
		printf("琼");
		carPlateProvince += "琼";
		break;
	case 21:
		printf("陕");
		carPlateProvince += "陕";
		break;
	case 22:
		printf("苏");
		carPlateProvince += "苏";
		break;
	case 23:
		printf("皖");
		carPlateProvince += "皖";
		break;
	case 24:
		printf("湘");
		carPlateProvince += "湘";
		break;
	case 25:
		printf("新");
		carPlateProvince += "新";
		break;
	case 26:
		printf("渝");
		carPlateProvince += "渝";
		break;
	case 27:
		printf("豫");
		carPlateProvince += "豫";
		break;
	case 28:
		printf("粤");
		carPlateProvince += "粤";
		break;
	case 29:
		printf("云");
		carPlateProvince += "云";
		break;
	case 30:
		printf("浙");
		carPlateProvince += "浙";
		break;
	case 31:
		printf("湘");
		carPlateProvince += "湘";
		break;
	case 32:
		printf("湘");
		carPlateProvince += "湘";
		break;
	case 33:
		printf("鲁");
		carPlateProvince += "鲁";
		break;
	case 34:
		printf("粤");
		carPlateProvince += "粤";
		break;
	}
	printf("\n");
}



//读取字母和数字模板
struct stu1
{
	char number;
	Mat image;
	double matchDegree;
};
struct stu1 second[49];
void readNumber()
{
	for (int i = 0; i < 10; i++) {
		second[i].image = imread("match\\" + to_string(i) + ".bmp", 0);
		second[i].number = 48 + i;
	}

	//读取字符
	{
		int i = 10;
		second[i].image = imread("match\\6a.bmp", 0);
		second[i].number = '6';
		i++;
		second[i].image = imread("match\\3a.bmp", 0);
		second[i].number = '3';
		i++;
		second[i].image = imread("match\\P1.bmp", 0);
		second[i].number = 'P';
		i++;
		second[i].image = imread("match\\8b.bmp", 0);
		second[i].number = '8';
		i++;
		second[i].image = imread("match\\K1.bmp", 0);
		second[i].number = 'K';
		i++;
		second[i].image = imread("match\\9a.bmp", 0);
		second[i].number = '9';
		i++;
		second[i].image = imread("match\\B2.bmp", 0);
		second[i].number = 'B';
		i++;
		second[i].image = imread("match\\G1.bmp", 0);
		second[i].number = 'G';
		i++;
		second[i].image = imread("match\\T1.bmp", 0);
		second[i].number = 'T';
		i++;
		second[i].image = imread("match\\B1.bmp", 0);
		second[i].number = 'B';
		i++;
		second[i].image = imread("match\\8a.bmp", 0);
		second[i].number = '8';
		i++;
		second[i].image = imread("match\\1a.bmp", 0);
		second[i].number = '1';
		i++;
		second[i].image = imread("match\\7a.bmp", 0);
		second[i].number = '7';
		i++;
		second[i].image = imread("match\\D1.bmp", 0);
		second[i].number = 'D';
		i++;
		second[i].image = imread("match\\0a.bmp", 0);
		second[i].number = '0';
		i++;
		second[i].image = imread("match\\A.bmp", 0);
		second[i].number = 'A';
		i++;
		second[i].image = imread("match\\B.bmp", 0);
		second[i].number = 'B';
		i++;
		second[i].image = imread("match\\C.bmp", 0);
		second[i].number = 'C';
		i++;
		second[i].image = imread("match\\D.bmp", 0);
		second[i].number = 'D';
		i++;
		second[i].image = imread("match\\E.bmp", 0);
		second[i].number = 'E';
		i++;
		second[i].image = imread("match\\F.bmp", 0);
		second[i].number = 'F';
		i++;
		second[i].image = imread("match\\G.bmp", 0);
		second[i].number = 'G';
		i++;
		second[i].image = imread("match\\H.bmp", 0);
		second[i].number = 'H';
		i++;
		second[i].image = imread("match\\J.bmp", 0);
		second[i].number = 'J';
		i++;
		second[i].image = imread("match\\K.bmp", 0);
		second[i].number = 'K';
		i++;
		second[i].image = imread("match\\L.bmp", 0);
		second[i].number = 'L';
		i++;
		second[i].image = imread("match\\M.bmp", 0);
		second[i].number = 'M';
		i++;
		second[i].image = imread("match\\N.bmp", 0);
		second[i].number = 'N';
		i++;
		second[i].image = imread("match\\P.bmp", 0);
		second[i].number = 'P';
		i++;
		second[i].image = imread("match\\Q.bmp", 0);
		second[i].number = 'Q';
		i++;
		second[i].image = imread("match\\R.bmp", 0);
		second[i].number = 'R';
		i++;
		second[i].image = imread("match\\S.bmp", 0);
		second[i].number = 'S';
		i++;
		second[i].image = imread("match\\T.bmp", 0);
		second[i].number = 'T';
		i++;
		second[i].image = imread("match\\U.bmp", 0);
		second[i].number = 'U';
		i++;
		second[i].image = imread("match\\V.bmp", 0);
		second[i].number = 'V';
		i++;
		second[i].image = imread("match\\W.bmp", 0);
		second[i].number = 'W';
		i++;
		second[i].image = imread("match\\X.bmp", 0);
		second[i].number = 'X';
		i++;
		second[i].image = imread("match\\Y.bmp", 0);
		second[i].number = 'Y';
		i++;
		second[i].image = imread("match\\Z.bmp", 0);
		second[i].number = 'Z';

	}


}


//识别其他字符
void MatchNumber()
{
	int rowMax = 40;
	int colMax = 20;
	int add = 0;
	int addTemp = 0;
	Mat absCutImage;
	double temp;
	int index = 0;
	int length = singleChar.size();
	int secondLength = end(second) - begin(second);
	//printf("数组长度2         %d   \n", secondLength);

	uchar* data;
	int q = 0;

	for (int y = 1; y < length; y++, add = 0, index = 0)
	{
		if (y > 6)                //防止多读
			break;
		//统计要读取字符的白色像素总值
		for (int i = 0; i < rowMax; i++)
		{
			data = singleChar[y].ptr<uchar>(i);
			for (int j = 0; j < colMax; j++)
			{
				if (data[j] == 255)
				{
					add++;
				}
			}
		}


		
		//逐个字符识别
		for (int x = 0; x < secondLength; x++, addTemp = 0)
		{

			absCutImage = abs(singleChar[y] - second[x].image);

			//统计相减之后的图像白色像素总值
			for (int i = 0; i < rowMax; i++)
			{
				data = absCutImage.ptr<uchar>(i);
				for (int j = 0; j < colMax; j++)
				{
					if (data[j] == 255)
					{
						addTemp++;
					}
				}
			}
			temp = 1.0 - 1.0*addTemp / add;
			if (temp <= 1 && temp > 0)
				second[x].matchDegree = temp;
			else
				second[x].matchDegree = 0;

			//获取最大匹配度对应索引index
			if (x > 0 && second[x].matchDegree > second[index].matchDegree)
				index = x;
		}
		absCutImage = abs(singleChar[y] - second[index].image);
		/*	namedWindow("图片相减后图"+to_string(y));
			imshow("图片相减后图" + to_string(y), absCutImage);*/
		printf("最大匹配度：  %lf\n", second[index].matchDegree);
		printf("对应字符：    %c\n", second[index].number);
		test[q] = second[index].number;
		//printf("\ntest11111           %c\n", test[q]);
		q++;
	}
	test[q] = '\0';
	//printf("\ntest22222           %c\n", test[q-1]);
	//cout<<  "xxxxxxxxxxxxxx"<<carPlate<<endl;
}

void VideoShow(Mat videoImage)
{
	/*if(videoFlag==0)*/
	//carPlate = "京A J9846";
	
		carPlate += test;
		carPlateProvince += carPlate;
	/*carPlate.copy(test,strlen(test));*/
	cout <<  carPlateProvince << endl;
	cout <<  carPlateProvince.length() << endl;
	if(carPlateProvince.length()<10)
		putTextZH(videoImage, "Not Plate!", Point(490, 20), Scalar(0, 0, 255), 30, "Arial", false, false);
	else
		putTextZH(videoImage, carPlateProvince.c_str(), Point(490, 20), Scalar(0, 0, 255), 30, "Arial", false, false);
	/*else if(videoFlag==1)*/

	namedWindow("提取车牌结果图");
	imshow("提取车牌结果图", videoImage);
	carPlateProvince = " ";
	carPlate = " ";
}


void GetStringSize(HDC hDC, const char* str, int* w, int* h)
{
	SIZE size;
	GetTextExtentPoint32A(hDC, str, strlen(str), &size);
	if (w != 0) *w = size.cx;
	if (h != 0) *h = size.cy;
}

void putTextZH(Mat &dst, const char* str, Point org, Scalar color, int fontSize, const char* fn, bool italic, bool underline)
{

	CV_Assert(dst.data != 0 && (dst.channels() == 1 || dst.channels() == 3));
	int x, y, r, b;
	if (org.x > dst.cols || org.y > dst.rows) return;
	x = org.x < 0 ? -org.x : 0;
	y = org.y < 0 ? -org.y : 0;
	LOGFONTA lf;
	lf.lfHeight = -fontSize;
	lf.lfWidth = 0;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfWeight = 5;
	lf.lfItalic = italic;   //斜体
	lf.lfUnderline = underline; //下划线
	lf.lfStrikeOut = 0;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = 0;
	lf.lfClipPrecision = 0;
	lf.lfQuality = PROOF_QUALITY;
	lf.lfPitchAndFamily = 0;
	strcpy_s(lf.lfFaceName, fn);

	HFONT hf = CreateFontIndirectA(&lf);
	HDC hDC = CreateCompatibleDC(0);
	HFONT hOldFont = (HFONT)SelectObject(hDC, hf);

	int strBaseW = 0, strBaseH = 0;
	int singleRow = 0;
	char buf[1 << 12];
	strcpy_s(buf, str);
	char *bufT[1 << 12];  // 这个用于分隔字符串后剩余的字符，可能会超出。
	//处理多行
	{
		int nnh = 0;
		int cw, ch;

		const char* ln = strtok_s(buf, "\n", bufT);
		while (ln != 0)
		{
			GetStringSize(hDC, ln, &cw, &ch);
			strBaseW = max(strBaseW, cw);
			strBaseH = max(strBaseH, ch);

			ln = strtok_s(0, "\n", bufT);
			nnh++;
		}
		singleRow = strBaseH;
		strBaseH *= nnh;
	}

	if (org.x + strBaseW < 0 || org.y + strBaseH < 0)
	{
		SelectObject(hDC, hOldFont);
		DeleteObject(hf);
		DeleteObject(hDC);
		return;
	}

	r = org.x + strBaseW > dst.cols ? dst.cols - org.x - 1 : strBaseW - 1;
	b = org.y + strBaseH > dst.rows ? dst.rows - org.y - 1 : strBaseH - 1;
	org.x = org.x < 0 ? 0 : org.x;
	org.y = org.y < 0 ? 0 : org.y;

	BITMAPINFO bmp = { 0 };
	BITMAPINFOHEADER& bih = bmp.bmiHeader;
	int strDrawLineStep = strBaseW * 3 % 4 == 0 ? strBaseW * 3 : (strBaseW * 3 + 4 - ((strBaseW * 3) % 4));

	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biWidth = strBaseW;
	bih.biHeight = strBaseH;
	bih.biPlanes = 1;
	bih.biBitCount = 24;
	bih.biCompression = BI_RGB;
	bih.biSizeImage = strBaseH * strDrawLineStep;
	bih.biClrUsed = 0;
	bih.biClrImportant = 0;

	void* pDibData = 0;
	HBITMAP hBmp = CreateDIBSection(hDC, &bmp, DIB_RGB_COLORS, &pDibData, 0, 0);

	CV_Assert(pDibData != 0);
	HBITMAP hOldBmp = (HBITMAP)SelectObject(hDC, hBmp);

	//color.val[2], color.val[1], color.val[0]
	SetTextColor(hDC, RGB(255, 255, 255));
	SetBkColor(hDC, 0);
	//SetStretchBltMode(hDC, COLORONCOLOR);

	strcpy_s(buf, str);
	const char* ln = strtok_s(buf, "\n", bufT);
	int outTextY = 0;
	while (ln != 0)
	{
		TextOutA(hDC, 0, outTextY, ln, strlen(ln));
		outTextY += singleRow;
		ln = strtok_s(0, "\n", bufT);
	}
	uchar* dstData = (uchar*)dst.data;
	int dstStep = dst.step / sizeof(dstData[0]);
	unsigned char* pImg = (unsigned char*)dst.data + org.x * dst.channels() + org.y * dstStep;
	unsigned char* pStr = (unsigned char*)pDibData + x * 3;
	for (int tty = y; tty <= b; ++tty)
	{
		unsigned char* subImg = pImg + (tty - y) * dstStep;
		unsigned char* subStr = pStr + (strBaseH - tty - 1) * strDrawLineStep;
		for (int ttx = x; ttx <= r; ++ttx)
		{
			for (int n = 0; n < dst.channels(); ++n) {
				double vtxt = subStr[n] / 255.0;
				int cvv = vtxt * color.val[n] + (1 - vtxt) * subImg[n];
				subImg[n] = cvv > 255 ? 255 : (cvv < 0 ? 0 : cvv);
			}

			subStr += 3;
			subImg += dst.channels();
		}
	}

	SelectObject(hDC, hOldBmp);
	SelectObject(hDC, hOldFont);
	DeleteObject(hf);
	DeleteObject(hBmp);
	DeleteDC(hDC);
}





