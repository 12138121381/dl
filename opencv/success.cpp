#include <opencv2/opencv.hpp>
#include<opencv2/imgproc/imgproc.hpp>
#include<opencv2/highgui/highgui.hpp>
#include"Globa.h"
#include <windows.h>
#include <string>

using namespace std;
using namespace cv;

void fillHole(const Mat srcBw, Mat &dstBw);           //��㷨
Mat cutOne(Mat cutImage);         //�߿��и��㷨
void CharCut(Mat srcImage);            //�����ַ��и��㷨
Mat Location(Mat srcImage);            //ͼ��ʶ���㷨
void SingleCharCut(Mat doubleImage, int k1, int k2);
void ShowChar();
void MatchProvince();
void MatchNumber();
void readProvince();
void readNumber();
void VideoShow(Mat videoImage);
void GetStringSize(HDC hDC, const char* str, int* w, int* h);
void putTextZH(Mat &dst, const char* str, Point org, Scalar color, int fontSize, const char* fn, bool italic, bool underline);




int flag_1;          //�ж��Ƿ���б���費��Ҫ���ζ�λ����
bool flag;       //�ж���ȡ�Ƿ�ɹ�
bool specialFlag = false;    //���Ƕ�׳���
int captureRead = 0;
int videoFlag = 0;
string carPlateProvince = " ";
string carPlate = " ";
char test[10];
vector<Mat>  singleChar;         //�ַ�ͼƬ����

int main(int argc, char *argv[])
{
	//��ʱ��ʼ
	double time0 = static_cast<double>(getTickCount());

	//��Ƶ����
	VideoCapture capture("1.mp4");
	Mat srcImage;
	Mat theFirst;
	int singleCharLength;

	//��ȡ�ַ�ͼƬ
	readProvince();
	readNumber();

	while (1) {
		capture >> srcImage;
		try {

			if (!srcImage.data) { printf("��Ƶʶ�����    \n"); return 0; }

			if (srcImage.rows >= srcImage.cols)
			{
				resize(srcImage, srcImage, Size(640, 640 * srcImage.rows / srcImage.cols));
			}
			else
			{
				resize(srcImage, srcImage, Size(400 * srcImage.cols / srcImage.rows, 400));
			}

			//���ƶ�λ

			theFirst = Location(srcImage);

			if (flag)
			{
				if (flag_1 == 1)                      //��ת��Ҫ�ٴζ�λȥ�����ӱ�
				{
					theFirst = Location(theFirst);
					flag_1 = 0;
				}
			}
			if (flag)
			{
				//�����и�(�и����±ߣ�ȥ������)
				theFirst = cutOne(theFirst);
				//�����ַ��и�
				CharCut(theFirst);
				singleCharLength = singleChar.size();
				printf("��ȡ�ַ�������                               %d\n", singleCharLength);
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
		//��ʱ30ms

	}
		//imwrite("match\\xxxxxx.bmp", singleChar[2]);


	                
	time0 = ((double)getTickCount() - time0) / getTickFrequency();
	cout << "����ʱ��" << time0 << "��" << endl;


	waitKey(0);
}

void fillHole(const Mat srcBw, Mat &dstBw)
{
	Size imageSize = srcBw.size();
	Mat Temp = Mat::zeros(imageSize.height + 2, imageSize.width + 2, srcBw.type());//��չͼ��
	srcBw.copyTo(Temp(Range(1, imageSize.height + 1), Range(1, imageSize.width + 1)));

	cv::floodFill(Temp, Point(0, 0), Scalar(255));

	Mat cutImg;//�ü���չ��ͼ��
	Temp(Range(1, imageSize.height + 1), Range(1, imageSize.width + 1)).copyTo(cutImg);

	dstBw = srcBw | (~cutImg);
}

Mat Location(Mat srcImage)
{
	//�жϱ����ظ�ֵ
	flag = false;

	//������ת����
	int	imageWidth, imageHeight;            //����ͼ��ĳ��Ϳ�
	imageWidth = srcImage.rows;                 //��ȡͼƬ�Ŀ�
	imageHeight = srcImage.cols;                 //��ȡͼ��ĳ�
	//!!!!!!!!!!!!!!!!!!!
	Mat blueROI = srcImage.clone();
	cvtColor(blueROI, blueROI, CV_BGR2HSV);
	//namedWindow("hsvͼ");
	//imshow("hsvͼ", blueROI);
	//��ֵ�˲�����
	medianBlur(blueROI, blueROI, 3);
	//namedWindow("medianBlurͼ");
	//imshow("medianBlurͼ", blueROI);
	//����ɫ�����ֵ��
	inRange(blueROI, Scalar(100, 130, 50), Scalar(124, 255, 255), blueROI);
	//namedWindow("blueͼ");
	//imshow("blueͼ", blueROI);

	Mat element1 = getStructuringElement(MORPH_RECT, Size(2, 2));     //size�������ٶ���Ӱ��
	morphologyEx(blueROI, blueROI, MORPH_OPEN, element1);
	//namedWindow("0��K�����ͼ��");
	//imshow("0��K�����ͼ��", blueROI);

	Mat element0 = getStructuringElement(MORPH_ELLIPSE, Size(10, 10));     //size�������ٶ���Ӱ��
	morphologyEx(blueROI, blueROI, MORPH_CLOSE, element0);
	//namedWindow("0�α������ͼ��");
	//imshow("0�α������ͼ��", blueROI);
	vector<vector<Point>> contours;

	findContours(blueROI, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);


	int cnt = contours.size();
	cout << "number of contours   " << cnt << endl;  //��ӡ��������
	if (cnt == 0)
	{
		if (!flag)        //����Ƶ����ʾ
		{
			cout << "ͼ���޳���       " << endl;
			//namedWindow("��ȡ���ƽ��ͼ");
			//imshow("��ȡ���ƽ��ͼ", srcImage);    //��ʾ���ս��ͼ
			VideoShow(srcImage);
			return  srcImage;
		}
	}



	double area;
	double longside, temp, shortside, long2short;
	float  angle = 0;
	Rect rect;
	RotatedRect box;    //����ת�ľ��κ���
	Point2f vertex[4];        //�ĸ�����

	Mat image = srcImage.clone();        //Ϊ������ʾ��׼��
	Mat  rgbCutImg;                       //���Ʋü�ͼ

	//box.points(vertex);            //��ȡ�����ĸ���������
	//length=arcLength(contour[i]);                        //��ȡ�����ܳ�
	//area=contourArea(contour[i]);                        //��ȡ�������
	//angle=box.angle;           //�õ�������б�Ƕ�

	for (int i = 0; i < cnt; i++)
	{
		area = contourArea(contours[i]);              //��ȡ�������
		if (area > 600 && area < 15000)     //�������������С�ж�
		{
			rect = boundingRect(contours[i]);    //������α߽�
			box = minAreaRect(contours[i]);      //��ȡ�����ľ���
			box.points(vertex);                  //��ȡ�����ĸ���������
			angle = box.angle;                   //�õ�������б�Ƕ�

			longside = sqrt(pow(vertex[1].x - vertex[0].x, 2) + pow(vertex[1].y - vertex[0].y, 2));
			shortside = sqrt(pow(vertex[2].x - vertex[1].x, 2) + pow(vertex[2].y - vertex[1].y, 2));
			if (shortside > longside)   //������ڳ��ᣬ��������
			{
				temp = longside;
				longside = shortside;
				shortside = temp;
				cout << "����" << endl;
			}
			else
				angle += 90;
			long2short = longside / shortside;
			if (long2short > 1.5 && long2short < 4.5)
			{
				flag = true;
				for (int i = 0; i < 4; ++i)       //���߿����������
					line(image, vertex[i], vertex[((i + 1) % 4) ? (i + 1) : 0], Scalar(0, 255, 0), 1, CV_AA);


				if (!flag_1)        //����Ƶ����ʾ
				{
					printf("��ȡ�ɹ�\n");
					/*namedWindow("��ȡ���ƽ��ͼ");
					imshow("��ȡ���ƽ��ͼ", image);  */  //��ʾ���ս��ͼ
					VideoShow(image);
				}

				rgbCutImg = srcImage(rect);
				//namedWindow("����ͼ");
				//imshow("����ͼ", rgbCutImg);//�ü�������	
				break;              //�˳�ѭ�������������б����任
			}
		}
	}
	cout << "��б�Ƕȣ�" << angle << endl;
	if (flag  &&  fabs(angle) > 0.8)        //���ƹ�ƫ��תһ��                ƫ�ƽǶ�Сʱ�ɲ����ã������ҵ����ʷ�Χ�ٸĽ�
	{
		flag_1 = 1;
		Mat RotractImg(imageWidth, imageHeight, CV_8UC1, Scalar(0, 0, 0));       //��б����ͼƬ
		Point2f center = box.center;           //��ȡ������������
		Mat M2 = getRotationMatrix2D(center, angle, 1);       //������ת�����ŵı任���� 
		warpAffine(srcImage, RotractImg, M2, srcImage.size(), 1, 0, Scalar(0));       //������б����
		//namedWindow("��б������ͼƬ",0);
		//imshow("��б������ͼƬ", RotractImg);
		rgbCutImg = RotractImg(rect);      //��ȡ���Ʋ�ɫ��Ƭ
		//namedWindow("����������");
		//imshow("����������", rgbCutImg);
			/*cout << "��������:" << box.center.x << "," << box.center.y << endl;*/
		return  rgbCutImg;
	}

	if (flag == false) {
		printf("��ȡʧ��\n");                      //���ڼӱ�Ե��ⷨʶ��
		if (!flag_1)        //����Ƶ����ʾ
		{
			/*namedWindow("��ȡ���ƽ��ͼ");
			imshow("��ȡ���ƽ��ͼ", image); */   //��ʾ���ս��ͼ
			VideoShow(image);
		}
	}
	return rgbCutImg;
}

Mat cutOne(Mat cutImage)
{
	//��ӡ���Ƴ���
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
	/*namedWindow("Resize����ͼ");
	imshow("Resize����ͼ", cutImage);*/
	int height = cutImage.rows;
	cout << "\tHeight:" << height << "\tWidth:" << 320 << endl;
	if (height < 86)
	{
		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!��������Ƕ�׳���!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		printf("Ƕ�׳���\n");
		specialFlag = true;

	}

	Mat whiteROI = cutImage.clone();

	if (specialFlag)
	{
		cvtColor(whiteROI, whiteROI, CV_BGR2HSV);
		//����ɫ�����ֵ��
		//inRange(whiteROI, Scalar(0, 0, 0), Scalar(130, 50, 245), whiteROI);      //���� S �����Ͷȿ���ʹhsv��ɫ��ⷶΧ����
		inRange(whiteROI, Scalar(0, 0, 0), Scalar(180, 100, 245), whiteROI);
		//namedWindow("specialFlagwhiteROIͼ");
		//imshow("specialFlagwhiteROIͼ", whiteROI);
	}
	else
	{
		GaussianBlur(whiteROI, whiteROI, Size(3, 3), 0, 0);
		/*namedWindow("GaussianBlur����ͼ");
		imshow("GaussianBlur����ͼ", whiteROI);*/

		cvtColor(whiteROI, whiteROI, CV_BGR2HSV);

		//medianBlur(whiteROI, whiteROI, 3);
		//namedWindow("Src_medianBlurͼ");
		//imshow("Src_medianBlurͼ", whiteROI);

		//����ɫ�����ֵ��
		//inRange(whiteROI, Scalar(0, 0, 10), Scalar(180, 30, 255), whiteROI);      //���� S �����Ͷȿ���ʹhsv��ɫ��ⷶΧ����
		inRange(whiteROI, Scalar(0, 0, 10), Scalar(180, 120, 255), whiteROI);
		//namedWindow("whiteROIͼ");
		//imshow("whiteROIͼ", whiteROI);
	}


	/*
	Mat element0 = getStructuringElement(MORPH_ELLIPSE, Size(4, 4));     //size�������ٶ���Ӱ��
	morphologyEx(whiteROI, whiteROI, MORPH_OPEN, element0);
	namedWindow("OPENͼ");
	imshow("OPENͼ", whiteROI);
	*/
	Mat dstImage = cutImage.clone();
	vector<vector<Point>> contours;
	findContours(whiteROI, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
	drawContours(dstImage, contours, -1, Scalar(0, 0, 255), 1);
	//namedWindow("�����ַ�����ʶ��ͼ");
	//imshow("�����ַ�����ʶ��ͼ", dstImage);
	inRange(dstImage, Scalar(0, 0, 255), Scalar(0, 0, 255), dstImage);
	//namedWindow("�ַ�������ͼ");
	//imshow("�ַ�������ͼ", dstImage);
	/*fillHole(dstImage, dstImage);
	namedWindow("�������ͼ");
	imshow("�������ͼ", dstImage);*/

	int row1 = 2;
	int row2 = dstImage.rows;
	int rowMax = dstImage.rows - 1;            //�����䣬��ֹԽ��
	int colMax = dstImage.cols - 1;            //�����䣬��ֹԽ��
	int addFirst = 10;
	int addFirst0 = 0;
	int addFirst1 = 0;
	int addFirst2 = 0;
	//���м�����
	//dstImage.at<uchar>(rowMax-1, colMax-1);
	//cout << "Width:" << j << endl;

	int addFirstTemp = addFirst;          //��һ����ʱ�Ѿ��ı���ֵ�����׺��ԣ���������

	uchar* data;

	//�ü����±ߡ�������������������������������������������������������������������������������������������������������������������������������������������
	//�ϱ�
	for (int i = 2; i < rowMax / 3; i++, addFirst1 = 0)                           //   6     �ոպ�
	{
		data = dstImage.ptr<uchar>(i);
		for (int j = 2; j < colMax; j++)
		{
			if (data[j] == 255)
			{
				addFirst1++;
			}
		}
		if (addFirst1 < addFirst)                       //ɸѡ��Сֵ������
		{
			row1 = i;
			addFirst = addFirst1 + 3;
			//cout << "��ͷ" << row1 << endl;
			//flag_x = 1;
		}
	}
	//�±�
	for (int i = rowMax - 2; i > rowMax - rowMax / 4; i--, addFirst2 = 0)                //   6     �ոպ�
	{
		data = dstImage.ptr<uchar>(i);
		for (int j = 2; j < colMax; j++)
		{
			if (data[j] == 255)
			{
				addFirst2++;
			}
		}
		if (addFirst2 < addFirstTemp)                       //ɸѡ��Сֵ������
		{
			row2 = i;
			addFirstTemp = addFirst2 + 3;
			//cout << "�е�" << row2 << endl;
			//flag_y = 1;
		}
	}


	int orow;
	orow = row2 - row1;
	Mat w_image;
	Mat  rgb_w_image;
	w_image = dstImage(Rect(0, row1, colMax, orow));
	rgb_w_image = cutImage(Rect(0, row1, colMax, orow));
	//namedWindow("�ü�����ͼ");
	//imshow("�ü�����ͼ", w_image);

	int rowMax_ALT = w_image.rows - 1;            //�����䣬��ֹԽ��(ע�⣬�ü������º�Ҫ����д�кͿ���Ϊ�кͿ��Ѿ��ı�)
	int colMax_ALT = w_image.cols - 1;            //�����䣬��ֹԽ�磨ע�⣬�ü������º�Ҫ����д�кͿ���Ϊ�кͿ��Ѿ��ı䣩
	int col_1 = 2;
	int col_2 = w_image.cols;
	int add = 2;
	int add1 = 0;
	int add2 = 0;

	int addTemp = add;        //��һ����ʱ�Ѿ��ı���ֵ�����׺��ԣ���������

	//�ü����ұߡ�������������������������������������������������������������������������������������������������������������������������������������������
	//���
	//for (int i = 0; i < colMax_ALT / 18; i++, add1 = 0)                           //       �ոպ�
	//{
	//	for (int j = 2; j < rowMax_ALT; j++)
	//	{
	//		data = dstImage.ptr<uchar>(j);
	//		if (data[i] == 255)
	//		{
	//			add1++;
	//		}
	//	}
	//	if (add1 < add)                       //ɸѡ��Сֵ������
	//	{
	//		col_1 = i;
	//		add = add1 + 1;
	//	}
	//}
	//�ұ�
	if (specialFlag)
	{
		for (int i = colMax_ALT; i > colMax_ALT - colMax_ALT / 18; i--, add2 = 0)                //        �ոպ�
		{
			for (int j = 2; j < rowMax_ALT; j++)
			{
				data = dstImage.ptr<uchar>(j);
				if (data[i] == 255)
				{
					add2++;
				}
			}
			if (add2 < addTemp)                       //ɸѡ��Сֵ������
			{
				col_2 = i;
				addTemp = add2 + 1;
				//cout << "�е�" << row2 << endl;
			}
		}
	}
	int o_col;
	o_col = col_2 - col_1;

	Mat H_image;
	H_image = w_image(Rect(col_1, 0, o_col, rowMax_ALT));
	rgb_w_image = rgb_w_image(Rect(col_1, 0, o_col, rowMax_ALT));
	//namedWindow("�ٲü�����ͼ");
	//imshow("�ٲü�����ͼ", H_image);
	//namedWindow("�ü����ͼ");
	//imshow("�ü����ͼ", rgb_w_image);

	return rgb_w_image;
}

void CharCut(Mat srcImage)
{
	resize(srcImage, srcImage, Size(320, 320 * srcImage.rows / srcImage.cols));
	//namedWindow("Resize����ͼ");
	//imshow("Resize����ͼ", srcImage);
	GaussianBlur(srcImage, srcImage, Size(3, 3), 0, 0);
	/*namedWindow("GaussianBlur����ͼ");
	imshow("GaussianBlur����ͼ", srcImage);
*/
	medianBlur(srcImage, srcImage, 3);
	//namedWindow("Src_medianBlurͼ");
	//imshow("Src_medianBlurͼ", srcImage);

	cvtColor(srcImage, srcImage, CV_BGR2HSV);


	//����ɫ�����ֵ��
	Mat doubleImage;
	//inRange(srcImage, Scalar(0, 0, 10), Scalar(180, 75, 255), doubleImage);      //���� S �����Ͷȿ���ʹhsv��ɫ��ⷶΧ����
	inRange(srcImage, Scalar(0, 0, 0), Scalar(180, 125, 245), doubleImage);
	namedWindow("doubleImageͼ");
	imshow("doubleImageͼ", doubleImage);

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


	//���Ƕ�׳��ƴ���
	if (specialFlag)
	{
		for (int i = 2; i < colMax; i++, addFirst = 0, add = 0)
		{
			for (int j = rowMax / 10.8; j < rowMax - rowMax / (10.8*t); j++)
			{
				data = doubleImage.ptr<uchar>(j);
				if (data[i - 1] == 255)
				{
					addFirst++;                             //ͳ��ǰһ��
				}
			}

			for (int j = rowMax / 10.8; j < rowMax - rowMax / (10.8*t); j++)
			{
				data = doubleImage.ptr<uchar>(j);
				if (data[i] == 255)
				{
					add++;                                  //ͳ�ƺ�һ��
				}
			}
			//ʡ���ַ��ֿ��и�
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
			}         //�и������ַ�
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
						else                                   //���Ƕ�׳����²����ӹ�����
						{
							i = k1;
							t -= 0.1;
						}
					}
					else
					{   //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!�����м�ָ���롮 1 ��!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
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
					{   //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!�����м�ָ���롮 1 ��!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
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
	rowMax = image.rows - 1;            //�����䣬��ֹԽ��
	int colMax = image.cols;            //�����䣬��ֹԽ��
	int addFirst = 2;
	int addFirst1 = 0;
	int addFirst2 = 0;
	uchar* data;
	//���м�����
	//image.at<uchar>(rowMax-1, colMax-1);
	//cout << "Width:" << j << endl;

	int addFirstTemp = addFirst;          //��һ����ʱ�Ѿ��ı���ֵ�����׺��ԣ���������

	//�ü����±ߡ�������������������������������������������������������������������������������������������������������������������������������������������
	//�ϱ�
	for (int i = 0; i < rowMax / 4; i++, addFirst1 = 0)                           //   6     �ոպ�
	{
		data = image.ptr<uchar>(i);
		for (int j = 0; j < colMax; j++)
		{
			if (data[j] == 255)
			{
				addFirst1++;
			}
		}
		if (addFirst1 < addFirst)                       //ɸѡ��Сֵ������
		{
			row1 = i;
			addFirst = addFirst1 + 1;
		}
	}

	//�±�
	for (int i = rowMax; i > rowMax - rowMax / 4; i--, addFirst2 = 0)                //   6     �ոպ�
	{
		data = image.ptr<uchar>(i);
		for (int j = 2; j < colMax; j++)
		{
			if (data[j] == 255)
			{
				addFirst2++;
			}
		}
		if (addFirst2 < addFirstTemp)                       //ɸѡ��Сֵ������
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

		resize(singleChar[i], singleChar[i], Size(20, 40));           //�ַ�ͼ���һ��

		//namedWindow(to_string(i) + "ͼ");
		//imshow(to_string(i) + "ͼ", singleChar[i]);
	}
}




//��ȡʡ��ģ��
struct  stu
{
	Mat image;
	double matchDegree;
};
struct  stu first[35];
void readProvince()
{
	int i = 0;
	//��ȡ�ַ�
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


//ʶ��ʡ���ַ�
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
	//printf("���鳤��1         %d\n",firstLength);
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
	namedWindow("ʡ��ͼƬ�����ͼ" + to_string(0));
	imshow("ʡ��ͼƬ�����ͼ" + to_string(0), absCutImage);*/

	printf("ʡ���ַ����ƥ��ȣ�  %lf\n", first[index].matchDegree);

	switch (index) {
	case 0:
		printf("��");
		carPlateProvince += "��";
		break;
	case 1:
		printf("��");
		carPlateProvince += "��";
		break;
	case 2:
		printf("��");
		carPlateProvince += "��";
		break;
	case 3:
		printf("��");
		carPlateProvince += "��";
		break;
	case 4:
		printf("��");
		carPlateProvince += "��";
		break;
	case 5:
		printf("��");
		carPlateProvince += "��";
		break;
	case 6:
		printf("��");
		carPlateProvince += "��";
		break;
	case 7:
		printf("��");
		carPlateProvince += "��";
		break;
	case 8:
		printf("��");
		carPlateProvince += "��";
		break;
	case 9:
		printf("��");
		carPlateProvince += "��";
		break;
	case 10:
		printf("��");
		carPlateProvince += "��";
		break;
	case 11:
		printf("��");
		carPlateProvince += "��";
		break;
	case 12:
		printf("��");
		carPlateProvince += "��";
		break;
	case 13:
		printf("��");
		carPlateProvince += "��";
		break;
	case 14:
		printf("��");
		carPlateProvince += "��";
		break;
	case 15:
		printf("³");
		carPlateProvince += "³";
		break;
	case 16:
		printf("��");
		carPlateProvince += "��";
		break;
	case 17:
		printf("��");
		carPlateProvince += "��";
		break;
	case 18:
		printf("��");
		carPlateProvince += "��";
		break;
	case 19:
		printf("��");
		carPlateProvince += "��";
		break;
	case 20:
		printf("��");
		carPlateProvince += "��";
		break;
	case 21:
		printf("��");
		carPlateProvince += "��";
		break;
	case 22:
		printf("��");
		carPlateProvince += "��";
		break;
	case 23:
		printf("��");
		carPlateProvince += "��";
		break;
	case 24:
		printf("��");
		carPlateProvince += "��";
		break;
	case 25:
		printf("��");
		carPlateProvince += "��";
		break;
	case 26:
		printf("��");
		carPlateProvince += "��";
		break;
	case 27:
		printf("ԥ");
		carPlateProvince += "ԥ";
		break;
	case 28:
		printf("��");
		carPlateProvince += "��";
		break;
	case 29:
		printf("��");
		carPlateProvince += "��";
		break;
	case 30:
		printf("��");
		carPlateProvince += "��";
		break;
	case 31:
		printf("��");
		carPlateProvince += "��";
		break;
	case 32:
		printf("��");
		carPlateProvince += "��";
		break;
	case 33:
		printf("³");
		carPlateProvince += "³";
		break;
	case 34:
		printf("��");
		carPlateProvince += "��";
		break;
	}
	printf("\n");
}



//��ȡ��ĸ������ģ��
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

	//��ȡ�ַ�
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


//ʶ�������ַ�
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
	//printf("���鳤��2         %d   \n", secondLength);

	uchar* data;
	int q = 0;

	for (int y = 1; y < length; y++, add = 0, index = 0)
	{
		if (y > 6)                //��ֹ���
			break;
		//ͳ��Ҫ��ȡ�ַ��İ�ɫ������ֵ
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


		
		//����ַ�ʶ��
		for (int x = 0; x < secondLength; x++, addTemp = 0)
		{

			absCutImage = abs(singleChar[y] - second[x].image);

			//ͳ�����֮���ͼ���ɫ������ֵ
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

			//��ȡ���ƥ��ȶ�Ӧ����index
			if (x > 0 && second[x].matchDegree > second[index].matchDegree)
				index = x;
		}
		absCutImage = abs(singleChar[y] - second[index].image);
		/*	namedWindow("ͼƬ�����ͼ"+to_string(y));
			imshow("ͼƬ�����ͼ" + to_string(y), absCutImage);*/
		printf("���ƥ��ȣ�  %lf\n", second[index].matchDegree);
		printf("��Ӧ�ַ���    %c\n", second[index].number);
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
	//carPlate = "��A J9846";
	
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

	namedWindow("��ȡ���ƽ��ͼ");
	imshow("��ȡ���ƽ��ͼ", videoImage);
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
	lf.lfItalic = italic;   //б��
	lf.lfUnderline = underline; //�»���
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
	char *bufT[1 << 12];  // ������ڷָ��ַ�����ʣ����ַ������ܻᳬ����
	//�������
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





