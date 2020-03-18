/*输出图片在final里，
输出文本一个，在当前目录
*/
#include <iostream>  
#include <opencv2/opencv.hpp>  
#include <string> 
#include <fstream>
#include<cmath>

#define MAX_AREA 45000//16300
#define MIN_AREA 15
#define A1 10000
#define A2 2098
#define R1 1.494
#define R2 1.547
#define R3 6.05
using namespace std;
using namespace cv;

template<typename T> string toString(const T& t){
	ostringstream oss;  //创建一个格式化输出流
	oss << t;             //把值传递如流中
	return oss.str();
}

void fillHole(const Mat srcBw, Mat &dstBw);
void all_black(Mat &src);
void coloring_rain(Mat &src, vector<vector<Point> > contours);
int data_writing(vector<Point2f> mass_centre, vector<float> long_radius, vector<float> short_radius);
void center_mass(vector<vector<Point> > contours, vector<Point2f> &mc);
int main(int argc, char* argv[])
{
	string str1 = "pic_out\\";
	string str3 = ".bmp";


	for (int num = 1; num <= 23; num ++ )
	{
	//int num =19;
		string str2 = toString(num);
		string FileName = str1 + str2 + str3;

		Mat img = imread(FileName);
		if (img.empty())
		{
			cout << "Error: Could not load image" << endl;
			return 0;
		}

		Mat gray;
		cvtColor(img, gray, CV_BGR2GRAY);

		int th[23][3] = {	
			{ 80, 120, 190 },
			{ 80, 119, 190 },
			{ 107, 142, 174 },
			{ 109, 144, 176 },
			{ 116, 160, 197 },
			{ 96, 141, 178 },
			{ 105, 150, 189 },
			{ 106, 145, 185 },
			{ 98, 139, 173 },
			{ 84, 119, 174 },
			{ 71, 104, 155 },
			{ 70, 102, 153 },
			{ 73, 107, 159 },
			{ 74, 108, 174 },
			{ 76, 116, 183 },
			{ 86, 125, 190 },
			{ 71, 104, 157 },
			{ 70, 102, 152 },
			{ 74, 108, 172 },
			{ 74, 109, 173 },
			{ 75, 112, 180 },
			{ 78, 119, 185 },
			{ 74, 110, 178 }
		};
		Mat dst, dst1, dst2, dst3, dst4;
		Mat re1, re2, re3, re4;

		/*这里已经得到otsu的阈值分割门限，由th[]数组确定*/
		threshold(gray, dst1, th[num-1][0], 255, CV_THRESH_BINARY);
		threshold(gray, dst2, th[num - 1][1], 255, CV_THRESH_BINARY);
		threshold(gray, dst3, th[num - 1][2], 255, CV_THRESH_BINARY);
		threshold(gray, dst4, 255, 255, CV_THRESH_BINARY);

		threshold(dst1, re1, th[num - 1][0], 255, CV_THRESH_BINARY);
		re2 = dst1 - dst2;
		re3 = dst2 - dst3;
		re4 = dst3 - dst4;

		Mat element_erode = getStructuringElement(MORPH_CROSS, Size(3, 3));//腐蚀核
		Mat element_dilate = getStructuringElement(MORPH_CROSS, Size(3, 3));//膨胀核

		erode(re1, re1, element_erode);
		erode(re2, re2, element_erode);
		erode(re3, re3, element_erode);
		erode(re4, re4, element_erode);
		
		//imwrite("erode.bmp", re1);
		/*----------------------------------设置筛选条件筛选连通域---------------------------------------------*/
		//连通域的储存数据，点集向量形式
		//hierarchy[i][0],hierarchy[i][1],hierarchy[i][2],hierarchy[i][3],分别表示的是第i条轮廓(contours[i])的下一条，前一条，包含的第一条轮廓(第一条子轮廓)和包含他的轮廓(父轮廓)。
		vector<vector<Point> > contours_a[4], contours_b[4];
		vector<Vec4i> hierarchy_a[4], hierarchy_b[4];

		if (th[num-1][0]>90)//加着一段的原因是，对于白底图像用findcontours时会在第一层找到一个包括整个边框的轮廓，对于黑底则不会，这里用一个简单的阈值分开。
		{
			threshold(re1, re1, 10, 255, CV_THRESH_BINARY_INV);

		}
		
		findContours(re1, contours_a[0], hierarchy_a[0], CV_RETR_LIST, CV_CHAIN_APPROX_NONE);
		findContours(re2, contours_a[1], hierarchy_a[1], CV_RETR_LIST, CV_CHAIN_APPROX_NONE);
		findContours(re3, contours_a[2], hierarchy_a[2], CV_RETR_LIST, CV_CHAIN_APPROX_NONE);
		findContours(re4, contours_a[3], hierarchy_a[3], CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

		all_black(re1);
		drawContours(re1, contours_a[0], -1, 255, CV_FILLED, 8);

		//imshow("2", re1);




		/*计算连通域面积*/
		vector<float>contours_a_Area[4], contours_b_Area[4];
		float Area_temp;
		for (int k = 0; k < 4; k++)
		{

			for (int i = 0; i < contours_a[k].size(); i++)
			{
				Area_temp = contourArea(contours_a[k][i]);//轮廓面积计算函数
				contours_a_Area[k].push_back(Area_temp);
			}
		}


		/*计算椭圆径长和排除连通域小于构成椭圆的连通域*/
		Mat drawing = Mat::zeros(gray.size(), CV_8UC3);
		vector<RotatedRect> minEllipse_b[4];//最小外接椭圆
		vector	<float>	average_radius[4];
		Scalar color = Scalar(0, 0, 255);
		float tempk;
		for (int k = 0; k < 4; k++)
		{

			for (int i = 0; i < contours_a[k].size(); i++)
			{
				if ((MIN_AREA<contours_a_Area[k][i]) && (MAX_AREA>contours_a_Area[k][i]))//面积筛选
				{

					contours_b[k].push_back(contours_a[k][i]);
					hierarchy_b[k].push_back(hierarchy_a[k][i]);
					contours_b_Area[k].push_back(contours_a_Area[k][i]);

					minEllipse_b[k].push_back(fitEllipse(Mat(contours_a[k][i])));
					//average_radius[k].push_back(sqrt(minEllipse[k][i].size.height*minEllipse[k][i].size.width));//椭圆平均半径
				}
				else if (MAX_AREA < contours_a_Area[k][i])
				{
					tempk = contours_a_Area[k][i];
				}
			}
		}

		all_black(re1);
		all_black(re2);
		all_black(re3);
		all_black(re4);
		drawContours(re1, contours_b[0], -1, 255, CV_FILLED, 8);
		drawContours(re2, contours_b[1], -1, 255, CV_FILLED, 8);
		drawContours(re3, contours_b[2], -1, 255, CV_FILLED, 8);
		drawContours(re4, contours_b[3], -1, 255, CV_FILLED, 8);

		/*imshow("未筛选一层.bmp", re1);
		imshow("未筛选二层.bmp", re2);
		imshow("未筛选三层.bmp", re3);
		imshow("未筛选四层.bmp", re4);

		/*计算椭圆长短轴乘积的开方*/
		float temp = 0;
		for (int k = 0; k < 4; k++)
		{

			for (int i = 0; i < contours_b[k].size(); i++)
			{
				if (MIN_AREA < contours_b_Area[k][i])
				{
					temp = minEllipse_b[k][i].size.height/minEllipse_b[k][i].size.width;
					average_radius[k].push_back(temp);
				}
			}
		}


		/*筛选条件，分为三条，符合的进入删除列表：
		【1】B_area>A1,B_ratio>R1
		【2】A2<B_area<A1,B_rattio>R2
		【3】B_ratio>R3    */

		//【1】B_area>A1,B_ratio>R1
		vector<int> cancel_num[4];
		for (int k = 0; k < 4; k++)
		{
			for (int i = 0; i < contours_b[k].size(); i++)
			{
				if ((average_radius[k][i]>R1) && (contours_b_Area[k][i]>A1))
				{
					cancel_num[k].push_back(i);
				}
			}



		}

		//【2】A2<B_area<A1,B_rattio>R2
		int flag = 0;
		for (int k = 0; k < 4; k++)
		{
			for (int i = 0; i < contours_b[k].size(); i++)
			{

				if ((average_radius[k][i]>R2) && (contours_b_Area[k][i]>A2) && (contours_b_Area[k][i] < A1))
				{
					for (int j = 0; j < cancel_num[k].size(); j++)
					{
						if (cancel_num[k][j] == i)
						{
							flag = 1;
							continue;
						}
					}

					if (flag == 0)
					{

						cancel_num[k].push_back(i);
					}
					flag = 0;
				}
			}

		}



		//【3】B_ratio>R3

		for (int k = 0; k < 4; k++)
		{
			for (int i = 0; i < contours_b[k].size(); i++)
			{
				if ((((minEllipse_b[k][i].size.width)*R3)<(minEllipse_b[k][i].size.height))|| minEllipse_b[k][i].size.height>295)
				{
					for (int j = 0; j < cancel_num[k].size(); j++)
					{
						if (cancel_num[k][j] == i)
						{
							flag = 1;
							continue;
						}
					}
					if (flag == 0)
					{

						cancel_num[k].push_back(i);
					}
					flag = 0;
				}
			}

		}


		/*将符合条件的连通域放入新的向量空间contours_c*/
		vector<vector<Point> > contours_c[4];
		vector<float>contours_c_Area[4];
		vector<RotatedRect> minEllipse_c[4];
		vector	<float>	average_radius_c[4];

		for (int k = 0; k < 4; k++)
		{
			for (int i = 0; i < contours_b[k].size(); i++)
			{
				for (int j = 0; j < cancel_num[k].size(); j++)
				{
					if (cancel_num[k][j] == i)
					{
						flag = 1;
						continue;
					}
				}
				if (flag == 0)
				{
					contours_c[k].push_back(contours_b[k][i]);
					contours_c_Area[k].push_back(contours_b_Area[k][i]);
					minEllipse_c[k].push_back(minEllipse_b[k][i]);
					average_radius_c[k].push_back(average_radius[k][i]);
				}
				flag = 0;
			}
		}

		all_black(re1);
		all_black(re2);
		all_black(re3);
		all_black(re4);
		drawContours(re1, contours_c[0], -1, 255, CV_FILLED, 8);
		drawContours(re2, contours_c[1], -1, 255, CV_FILLED, 8);
		drawContours(re3, contours_c[2], -1, 255, CV_FILLED, 8);
		drawContours(re4, contours_c[3], -1, 255, CV_FILLED, 8);

		dilate(re1, re1, element_dilate);
		dilate(re2, re2, element_dilate);
		dilate(re3, re3, element_dilate);
		dilate(re4, re4, element_dilate);
		Mat re5 = re1 + re2 + re3 + re4;
		fillHole(re5, re5);

		vector<vector<Point>>contours_d;
		vector<Vec4i> hierarchy_d;
		vector<RotatedRect> minEllipse_d;
		findContours(re5, contours_d, hierarchy_d, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
		drawContours(re5, contours_d, -1, 255, CV_FILLED, 8);
		/*imshow("一层.bmp", re1);
		imshow("二层.bmp", re2);
		imshow("三层.bmp", re3);
		imshow("四层.bmp", re4);
		imshow("合成.bmp", re5);

		/*imwrite("temp\\一层.bmp", re1);
		imwrite("temp\\二层.bmp", re2);
		imwrite("temp\\三层.bmp", re3);
		imwrite("temp\\四层.bmp", re4);
		imwrite("temp\\合成.bmp", re5);*/
		if (num == 16)
		{

			imwrite("一层.bmp", re1);
			imwrite("二层.bmp", re2);
			imwrite("三层.bmp", re3);
			imwrite("四层.bmp", re4);
			imwrite("合成.bmp", re5);
			Mat drawing = Mat::zeros(re5.size(), CV_8UC3);
			all_black(drawing);
			vector<float> long_axe, short_axe;
			for (int i = 0; i < contours_d.size(); i++)
			{
				minEllipse_d.push_back(fitEllipse(Mat(contours_d[i])));
				ellipse(drawing, minEllipse_d[i], color, 2, 8);
				long_axe.push_back(minEllipse_d[i].size.height);
				short_axe.push_back(minEllipse_d[i].size.width);
			}

			imwrite("t椭圆.bmp", drawing);
			
			vector<Point2f> mass_centre;//质心

			center_mass(contours_d, mass_centre);//计算质心
			data_writing(mass_centre, long_axe, short_axe);//写入文本文件
		}

		Mat original = imread(FileName);
		coloring_rain(original, contours_d);
	
	//	imshow("color", original);


		string outFile = "final//final_" + toString(num) + ".bmp";
		imwrite(outFile,original);

		}
	//	waitKey(0);
	return 0;
}



/*孔洞填充函数，对合成后的连通域使用*/
void fillHole(const Mat srcBw, Mat &dstBw)
{
	Size m_Size = srcBw.size();
	Mat Temp = Mat::zeros(m_Size.height + 2, m_Size.width + 2, srcBw.type());//延展图像
	srcBw.copyTo(Temp(Range(1, m_Size.height + 1), Range(1, m_Size.width + 1)));

	cv::floodFill(Temp, Point(0, 0), Scalar(255));

	Mat cutImg;//裁剪延展的图像
	Temp(Range(1, m_Size.height + 1), Range(1, m_Size.width + 1)).copyTo(cutImg);

	dstBw = srcBw | (~cutImg);
}

void all_black(Mat &src)
{
	for (int i = 0; i < src.rows; i++)
	{
		for (int j = 0; j < src.cols; j++)
		{
			src.at<uchar>(i, j) = 0;

		}
	}
}

void coloring_rain(Mat &src, vector<vector<Point> > contours)
{
	Mat imBlue, imRed, imGreen;
	vector<Mat> channels;
	Mat  imageBlueChannel;
	split(src, channels);//分离色彩通道

	imBlue = channels.at(0);
	imRed = channels.at(2);
	imGreen = channels.at(1);


	drawContours(imRed, contours, -1, CV_RGB(255, 255, 255), CV_FILLED, 8);//通道染色
	//imshow("t", imRed);
	merge(channels, src);
}

int data_writing(vector<Point2f> mass_centre, vector<float> long_radius, vector<float> short_radius)//文本输出函数
{
	ofstream in_file;
	in_file.open("out.txt");
	in_file << "雨滴数量：\t" << long_radius.size() << "\n";
	in_file << "雨滴质心位置\t" << "长轴\t" << "短轴\t" << "\n";

	for (int i = 0; i < long_radius.size(); i++)
	{
		in_file << mass_centre[i] << "\t" << long_radius[i] << "\t" << short_radius[i] << "\t" << "\n";
	}


	return 0;
}

void center_mass(vector<vector<Point> > contours, vector<Point2f> &mc)//计算质心的函数
{
	vector<Moments> mu(contours.size());
	int i;
	float temp1, temp2;
	Point2f resu;
	for (i = 0; i < contours.size(); i++)
	{
		mu[i] = moments(contours[i], false);
	}

	for (i = 0; i < contours.size(); i++)
	{
		temp1 = mu[i].m10 / mu[i].m00;
		temp2 = mu[i].m01 / mu[i].m00;
		resu = Point(temp1, temp2);
		mc.push_back(resu);
	}
}