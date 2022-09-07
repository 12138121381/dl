#include<iostream> ///////////////////腐蚀用于分割(isolate)独立的图像元素，膨胀用于连接(join)相邻的元素
#include<opencv2/opencv.hpp>
#include<fstream>   
#include<Windows.h>

using namespace std;
using namespace cv;


///图像预处理
Mat Image_Preprocessing(Mat temp)//图像预处理
{
	Mat kernel = getStructuringElement(MORPH_RECT, Size(25, 25), Point(-1, -1));////getStructuringElement函数会返回指定形状和尺寸的结构元素。
	Mat open_gray_blur_Image;
	morphologyEx(temp, open_gray_blur_Image, MORPH_OPEN, kernel);////////////利用morphologyEx更好的进行腐蚀，膨胀，和开操作
																 //（腐蚀用于分割(isolate)独立的图像元素，膨胀用于连接(join)相邻的元素）
                                                                   /*各种操作的效果
                                                                   ●MORPH_ERODE（腐蚀）
                                                                   跟erode（腐蚀）函数效果一样
                                                                   ●MORPH_DILATE（膨胀）
                                                                   跟dilate（膨胀）函数效果一样
                                                                   ●MORPH_OPEN（开）*/
	Mat rst;
	subtract(temp, open_gray_blur_Image, rst, Mat());////////////矩阵相减，有点懵逼
	imshow("rst", rst);
	Mat Canny_Image;
	Canny(rst, Canny_Image, 400, 200, 3);/////////灰度化，直接变黑白，有点像二值化
	imshow("Canny_Image", Canny_Image);
	return Canny_Image;
}



//形态学处理
Mat Morphological_Processing(Mat temp)//形态学处理
{
	//图片膨胀处理
	Mat dilate_image, erode_image;
	//自定义核：进行 x 方向的膨胀腐蚀
	Mat elementX = getStructuringElement(MORPH_RECT, Size(25, 1));////getStructuringElement函数会返回指定形状和尺寸的结构元素。
	Mat elementY = getStructuringElement(MORPH_RECT, Size(1, 19));
	Point point(-1, -1);
	dilate(temp, dilate_image, elementX, point, 2);//dilate()函数可以对输入图像用特定结构元素进行膨胀操作，该结构元素确定膨胀操作过程中的邻域的形状，各点像素值将被替换为对应邻域上的最大值：
	erode(dilate_image, erode_image, elementX, point, 4);////跟膨胀操作一样
	dilate(erode_image, dilate_image, elementX, point, 2);
	//自定义核：进行 Y 方向的膨胀腐蚀
	erode(dilate_image, erode_image, elementY, point, 1);
	dilate(erode_image, dilate_image, elementY, point, 2);
	//平滑处理
	Mat median_Image;
	medianBlur(dilate_image, median_Image, 15);//中值滤波
	medianBlur(median_Image, median_Image, 15);
	imshow("中值滤波", median_Image);
	return median_Image;
}




//车牌定位
Mat Locate_License_Plate(Mat temp, Mat src, Mat gray_src)       //p6,p0,p1
{
	vector<vector<Point>> contours;
	findContours(temp, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);////////findContours函数 —— 在二值图像（binary image）中寻找轮廓（contour）
	//画出轮廓
	drawContours(temp, contours, -1, Scalar(255), 1);//////////画轮廓
	//轮廓表示为一个矩形
	Mat Roi;
	for (int i = 0; i < contours.size(); i++)
	{
		RotatedRect rect = minAreaRect(Mat(contours[i]));           ///////RotatedRect表示平面上的旋转矩形
		Point2f p[4];
		rect.points(p);                                            ///Point2f表示Point类的两个数据x,y为float类型；
		double axisLongTemp = 0.0, axisShortTemp = 0.0;            //矩形的长边和短边
		axisLongTemp = sqrt(pow(p[1].x - p[0].x, 2) + pow(p[1].y - p[0].y, 2));  //计算长轴
		axisShortTemp = sqrt(pow(p[2].x - p[1].x, 2) + pow(p[2].y - p[1].y, 2)); //计算短轴
		double LengthTemp;//中间变量，temp
		if (axisShortTemp > axisLongTemp)//若短轴大于长轴，交换数据
		{
			LengthTemp = axisLongTemp;
			axisLongTemp = axisShortTemp;
			axisShortTemp = LengthTemp;
		}
		double rectArea = axisLongTemp*axisShortTemp;//计算矩形面积
		double Area = contourArea(Mat(contours[i]));//轮廓面积
		double rectDegree = Area / rectArea;//计算矩形度
		if (axisLongTemp / axisShortTemp >= 2.2 && axisLongTemp / axisShortTemp <= 5.1 && rectDegree > 0.63 && rectDegree < 1.37 && rectArea>2000 && rectArea < 50000)//通过划定长宽比，矩形度以及矩形面积的变化范围划定车牌区域（该部分可视实际情况而调整）
		{
			for (int i = 0; i < 4; i++)       //划线框出车牌区域
				line(src, p[i], p[((i + 1) % 4) ? (i + 1) : 0], Scalar(0, 0, 255), 2, 8, 0);////划线

			float width_height = (float)rect.size.width / (float)rect.size.height;
			float angle = rect.angle;
			if (width_height < 1)//处理图像中旋转角度大于90度的车牌
				angle = angle - 90;
			Mat rotMat = getRotationMatrix2D(rect.center, angle, 1);//获得矩形的旋转矩阵
			Mat warpImg;
			warpAffine(gray_src, warpImg, rotMat, src.size(), INTER_CUBIC);//仿射变换是指在向量空间中进行一次线性变换(乘以一个矩阵)并加上一个平移(加上一个向量)，变换为另一个向量空间的过程
			imshow("仿射变换", warpImg);
			//图像切割
			Size minRectSize = rect.size;
			if (width_height < 1)
				swap(minRectSize.width, minRectSize.height);
			getRectSubPix(warpImg, minRectSize, rect.center, Roi);//从原图像中提取一个需要的矩形区域图像。（输入图像，获取需要区域矩形的大小，需要区域矩形在原图像中的位置（即需要区域矩形的中心点坐标），输出的图像）


		}
	}
	//imshow("test", src);
	imshow("车牌提取结果", Roi);
	return Roi;
}




Mat Affine_Transform(Mat temp)///////////////关于仿射矩阵
{
	Mat warp_dstImage = Mat::zeros(100, 500, temp.type());
	Point2f srcTri[3];
	Point2f dstTri[3];
	//设置三个点来计算仿射变换
	srcTri[0] = Point2f(0, 0);
	srcTri[1] = Point2f(temp.cols, 0);
	srcTri[2] = Point2f(0, temp.rows);

	dstTri[0] = Point2f(0, 0);
	dstTri[1] = Point2f(500, 0);
	dstTri[2] = Point2f(0, 100);
	//计算仿射变换矩阵
	Mat warp_mat(2, 3, CV_32FC1);
	warp_mat = getAffineTransform(srcTri, dstTri);
	//对加载图形进行仿射变换操作
	warpAffine(temp, warp_dstImage, warp_mat, warp_dstImage.size());
	return warp_dstImage;
}




//切除车牌边框
Mat Remove_Vertial_Border(Mat temp)
{
	Mat vline = getStructuringElement(MORPH_RECT, Size(1, temp.rows), Point(-1, -1));//getStructuringElement函数会返回指定形状和尺寸的结构元素。

Mat getStructuringElement(int shape, Size esize, Point anchor = Point(-1, -1));   
	Mat dst1, temp1;                                                                  //这个函数的第一个参数表示内核的形状，有三种形状可以选择。矩形：MORPH_RECT;//交叉形：MORPH_CORSS; //椭圆形：MORPH_ELLIPSE;
	erode(temp, temp1, vline);                                                        //函数形式与膨胀相似
	dilate(temp1, dst1, vline);                                                       //dilate()函数可以对输入图像用特定结构元素进行膨胀操作，该结构元素确定膨胀操作过程中的邻域的形状，各点像素值将被替换为对应邻域上的最大值：
	namedWindow("提取垂直线", WINDOW_AUTOSIZE);                                       //namedWindow（）的功能就是新建一个显示窗口。可以指定窗口的类型。
    imshow("提取垂直线", dst1);
	subtract(temp, dst1, temp, Mat());
	imshow("切割车牌垂直边框结果", temp);
	return temp;
}
Mat Remove_Horizon_Border(Mat temp)
{
	Mat hline = getStructuringElement(MORPH_RECT, Size(60, 1), Point(-1, -1));//矩形形状为：1*src.cols
	Mat dst1, temp1;
	erode(temp, temp1, hline);
	dilate(temp1, dst1, hline);
	namedWindow("提取水平线", WINDOW_AUTOSIZE);
	imshow("提取水平线", dst1);
	subtract(temp, dst1, temp, Mat());
	imshow("切割车牌水平边框结果", temp);
	return temp;
}




Mat Horizon_Cut(Mat temp)
{
	int *counter_y = new int[temp.rows];
	for (int i = 0; i < temp.rows; i++)
		counter_y[i] = 0;
	for (int row = 0; row < temp.rows; row++)
	{
		int count = 0;
		for (int col = 0; col < temp.cols; col++)
		{
			if (temp.at<uchar>(row, col) == 255)
			{
				count++;
			}
		}
		if (count > 50)
		{
			counter_y[row] = 1;
		}
	}
	for (int i = 0; i < temp.rows; i++)
		cout << counter_y[i] << '\t';// = 0;
	cout << endl;
	int count_temp = 0;
	int *record = new int[temp.rows];
	for (int i = 0; i < temp.rows; i++)
		record[i] = 0;
	for (int i = 0; i < temp.rows; i++)
	{
		if (counter_y[i] == 1)
		{
			count_temp++;
			record[i] = count_temp;
		}
		else
			count_temp = 0;
	}
	int max = record[0];
	int index = 0;
	for (int i = 1; i < temp.rows; i++)
	{
		if (max < record[i])
		{
			max = record[i];
			index = i;
		}
	}
	int index_row_begin = index - max + 1;
	int index_row_end = index;
	cout << index_row_begin << endl << index_row_end << endl;
	int height = index_row_end - index_row_begin;
	Mat image_preprocess = Mat::zeros(height, temp.cols, CV_8UC1);
	for (int row = 0; row < image_preprocess.rows; row++)
	{
		for (int col = 0; col < image_preprocess.cols; col++)
		{
			image_preprocess.at<uchar>(row, col) = temp.at<uchar>(row + index_row_begin, col);
		}
	}
	imshow("image_preprocess", image_preprocess);
	return image_preprocess;
}
void Locate_String(int *x_begin, int *x_end, Mat temp)
{
	int *counter_x = new int[temp.cols];//记录每一列的白像素个数
	for (int i = 0; i < temp.cols; i++)
		counter_x[i] = 0;
	for (int col = 0; col < temp.cols; col++)
	{
		int count = 0;
		for (int row = 0; row < temp.rows; row++)
		{
			if (temp.at<uchar>(row, col) == 255)
			{
				count++;
			}
		}
		counter_x[col] = count;
	}
	int index_col = 0;
	int number_width = 0;
	for (int i = 0; i < temp.cols - 1; i++)
	{
		if (counter_x[i] >= 6)//此处阈值可视情况调整
		{
			number_width++;
			if (number_width > 10)//此处阈值可视情况调整
			{
				x_end[index_col] = i;
				x_begin[index_col] = i - number_width + 1;
				if (counter_x[i + 1] < 6)//此处阈值可视情况调整
				{
					number_width = 0;
					index_col++;
				}
			}
		}
		else
		{
			number_width = 0;
		}
		if (index_col >= 8)
			break;
	}
}






void Recognize_Lisence(int *x_begin, int *x_end, Mat temp)
{
	int cycle_index = 0;
	for (int i = 0; i < 8; i++)
	{
		if (x_end[i] > 0)
			cycle_index++;
	}
	for (int i = 0; i < cycle_index; i++)
	{
		float error[27] = { 0 };
		//	//picture1是二值图像
		Mat picture1 = Mat::zeros(temp.rows, x_end[i] - x_begin[i], temp.type());
		for (int row = 0; row < picture1.rows; row++)
		{
			for (int col = 0; col < picture1.cols; col++)
			{
				picture1.at<uchar>(row, col) = temp.at<uchar>(row, col + x_begin[i]);
			}
		}
		Mat NUM[27];//字符匹配模板
		for (int i = 0; i < 27; i++)
		{
			stringstream stream;
			stream << "pictures/num_";
			stream << i;
			stream << ".bmp";
			String name = stream.str();
			NUM[i] = imread(name);
			if (NUM[i].empty())
			{
				cout << "未能读取" << name << endl;
			}
			cvtColor(NUM[i], NUM[i], COLOR_BGR2GRAY);
			threshold(NUM[i], NUM[i], 0, 255, THRESH_BINARY);

			Point2f srcTri[3];
			Point2f dstTri[3];
			Mat warp_mat(2, 3, CV_32FC1);
			//创建仿射变换目标图像与原图像尺寸类型相同
			Mat result = Mat::zeros(picture1.rows, picture1.cols, picture1.type());
			//设置三个点来计算仿射变换
			srcTri[0] = Point2f(0, 0);
			srcTri[1] = Point2f(NUM[i].cols, 0);
			srcTri[2] = Point2f(0, NUM[i].rows);
			dstTri[0] = Point2f(0, 0);
			dstTri[1] = Point2f(picture1.cols, 0);
			dstTri[2] = Point2f(0, picture1.rows);
			//计算仿射变换矩阵
			warp_mat = getAffineTransform(srcTri, dstTri);
			//对加载图形进行仿射变换操作
			warpAffine(NUM[i], result, warp_mat, picture1.size());
			threshold(result, result, 0, 255, THRESH_BINARY_INV);
			float error_sum = 0;
			float error_temp = 0;
			for (int row = 0; row < result.rows; row++)
			{
				for (int col = 0; col < result.cols; col++)
				{
					error_temp = picture1.at<uchar>(row, col) - result.at<uchar>(row, col);
					error_sum = error_sum + pow(error_temp, 2);
				}
			}
			error[i] = error_sum / (picture1.rows*picture1.cols * 255);
		}
		float min_error = error[0];
		int Index = 0;
		for (int i = 1; i < 27; i++)
		{
			if (min_error > error[i])
			{
				min_error = error[i];
				Index = i;
			}
		}
		if (Index == 10)
			cout << "E" << '\t';
		else if (Index == 11)
			cout << "V" << '\t';
		else if (Index == 12)
			cout << "苏" << '\t';
		else if (Index == 13)
			cout << "沪" << '\t';
		else if (Index == 14)
			cout << "B" << '\t';
		else if (Index == 15)
			cout << "S" << '\t';
		else if (Index == 16)
			cout << "京" << '\t';
		else if (Index == 17)
			cout << "N" << '\t';
		else if (Index == 18)
			cout << "J" << '\t';
		else if (Index == 19)
			cout << "P" << '\t';
		else if (Index == 20)
			cout << "A" << '\t';
		else if (Index == 21)
			cout << "浙" << '\t';
		else if (Index == 22)
			cout << "G" << '\t';
		else if (Index == 23)
			cout << "U" << '\t';
		else if (Index == 24)
			cout << "豫" << '\t';
		else if (Index == 25)
			cout << "K" << '\t';
		else if (Index == 26)
			cout << "陕" << '\t';
		else if (Index >= 0 && Index <= 9)
			cout << Index << '\t';
	}
	cout << endl;
}




int main()
{
	Mat p1;//gray_src
	Mat p0 = imread("C:/Users/86157/Desktop/opencv练习/756542b89152a0995eef3e1745adc585_watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80NDYxMzA2Mw==,size_16,color_FFFFFF,t_70#pic_center.jpg");//src
	//////灰度化
	cvtColor(p0, p1, COLOR_BGR2GRAY);
	Mat p2;//gray_blur_Image
	////二值化
	threshold(p1, p2, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
	///////高斯模糊
	Mat p3;//gray_blur_Image
	GaussianBlur(p2, p3, Size(3, 3), 0, 0);
	/////////图像预处理
	Mat p4 = Image_Preprocessing(p3);//Canny_Image
	///////形态学处理
	Mat p5 = Morphological_Processing(p4);//median_Image
	//车牌定位:矩形轮廓查找与筛选
	Mat p6;//contour_Image
	//查找轮廓会改变源图像信息，需要重新拷贝图像
	p6 = p5.clone();////////                         ////////克隆
	Mat p7 = Locate_License_Plate(p6, p0, p1);////p7=Roi
	//创建仿射变换目标图像与原图像尺寸类型相同
	Mat p8 = Affine_Transform(p7);//p8=warp _dstImage
	Mat p9;///bin_warp_destImage
	threshold(p8, p9, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
	imshow("同一尺寸的二值图像", p9);
	//车牌识别
	//切除车牌的水平与垂直边框
	p9 = Remove_Vertial_Border(p9);
	p9 = Remove_Horizon_Border(p9);
	//除去车牌号码以外的冗余部分
	Mat p10 = Horizon_Cut(p9);
	//车牌号码分割并显示分割结果
	int *x_begin = new int[8];
	int *x_end = new int[8];
	for (int i = 0; i < 8; i++)
	{
		x_begin[i] = 0;
		x_end[i] = 0;
	}
	//Locate_String(x_begin, x_end, license);
	//Draw_Result(x_begin, x_end, license);
	//车牌号码识别
	cout << "车牌号识别结果：" << endl;
	//Recognize_Lisence(x_begin, x_end, license);
	delete[] x_begin;
	delete[] x_end;
	waitKey(0);
}



