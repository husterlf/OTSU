/*���ͼƬ��final�
����ı�һ�����ڵ�ǰĿ¼
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
	ostringstream oss;  //����һ����ʽ�������
	oss << t;             //��ֵ����������
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

		/*�����Ѿ��õ�otsu����ֵ�ָ����ޣ���th[]����ȷ��*/
		threshold(gray, dst1, th[num-1][0], 255, CV_THRESH_BINARY);
		threshold(gray, dst2, th[num - 1][1], 255, CV_THRESH_BINARY);
		threshold(gray, dst3, th[num - 1][2], 255, CV_THRESH_BINARY);
		threshold(gray, dst4, 255, 255, CV_THRESH_BINARY);

		threshold(dst1, re1, th[num - 1][0], 255, CV_THRESH_BINARY);
		re2 = dst1 - dst2;
		re3 = dst2 - dst3;
		re4 = dst3 - dst4;

		Mat element_erode = getStructuringElement(MORPH_CROSS, Size(3, 3));//��ʴ��
		Mat element_dilate = getStructuringElement(MORPH_CROSS, Size(3, 3));//���ͺ�

		erode(re1, re1, element_erode);
		erode(re2, re2, element_erode);
		erode(re3, re3, element_erode);
		erode(re4, re4, element_erode);
		
		//imwrite("erode.bmp", re1);
		/*----------------------------------����ɸѡ����ɸѡ��ͨ��---------------------------------------------*/
		//��ͨ��Ĵ������ݣ��㼯������ʽ
		//hierarchy[i][0],hierarchy[i][1],hierarchy[i][2],hierarchy[i][3],�ֱ��ʾ���ǵ�i������(contours[i])����һ����ǰһ���������ĵ�һ������(��һ��������)�Ͱ�����������(������)��
		vector<vector<Point> > contours_a[4], contours_b[4];
		vector<Vec4i> hierarchy_a[4], hierarchy_b[4];

		if (th[num-1][0]>90)//����һ�ε�ԭ���ǣ����ڰ׵�ͼ����findcontoursʱ���ڵ�һ���ҵ�һ�����������߿�����������ںڵ��򲻻ᣬ������һ���򵥵���ֵ�ֿ���
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




		/*������ͨ�����*/
		vector<float>contours_a_Area[4], contours_b_Area[4];
		float Area_temp;
		for (int k = 0; k < 4; k++)
		{

			for (int i = 0; i < contours_a[k].size(); i++)
			{
				Area_temp = contourArea(contours_a[k][i]);//����������㺯��
				contours_a_Area[k].push_back(Area_temp);
			}
		}


		/*������Բ�������ų���ͨ��С�ڹ�����Բ����ͨ��*/
		Mat drawing = Mat::zeros(gray.size(), CV_8UC3);
		vector<RotatedRect> minEllipse_b[4];//��С�����Բ
		vector	<float>	average_radius[4];
		Scalar color = Scalar(0, 0, 255);
		float tempk;
		for (int k = 0; k < 4; k++)
		{

			for (int i = 0; i < contours_a[k].size(); i++)
			{
				if ((MIN_AREA<contours_a_Area[k][i]) && (MAX_AREA>contours_a_Area[k][i]))//���ɸѡ
				{

					contours_b[k].push_back(contours_a[k][i]);
					hierarchy_b[k].push_back(hierarchy_a[k][i]);
					contours_b_Area[k].push_back(contours_a_Area[k][i]);

					minEllipse_b[k].push_back(fitEllipse(Mat(contours_a[k][i])));
					//average_radius[k].push_back(sqrt(minEllipse[k][i].size.height*minEllipse[k][i].size.width));//��Բƽ���뾶
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

		/*imshow("δɸѡһ��.bmp", re1);
		imshow("δɸѡ����.bmp", re2);
		imshow("δɸѡ����.bmp", re3);
		imshow("δɸѡ�Ĳ�.bmp", re4);

		/*������Բ������˻��Ŀ���*/
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


		/*ɸѡ��������Ϊ���������ϵĽ���ɾ���б�
		��1��B_area>A1,B_ratio>R1
		��2��A2<B_area<A1,B_rattio>R2
		��3��B_ratio>R3    */

		//��1��B_area>A1,B_ratio>R1
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

		//��2��A2<B_area<A1,B_rattio>R2
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



		//��3��B_ratio>R3

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


		/*��������������ͨ������µ������ռ�contours_c*/
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
		/*imshow("һ��.bmp", re1);
		imshow("����.bmp", re2);
		imshow("����.bmp", re3);
		imshow("�Ĳ�.bmp", re4);
		imshow("�ϳ�.bmp", re5);

		/*imwrite("temp\\һ��.bmp", re1);
		imwrite("temp\\����.bmp", re2);
		imwrite("temp\\����.bmp", re3);
		imwrite("temp\\�Ĳ�.bmp", re4);
		imwrite("temp\\�ϳ�.bmp", re5);*/
		if (num == 16)
		{

			imwrite("һ��.bmp", re1);
			imwrite("����.bmp", re2);
			imwrite("����.bmp", re3);
			imwrite("�Ĳ�.bmp", re4);
			imwrite("�ϳ�.bmp", re5);
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

			imwrite("t��Բ.bmp", drawing);
			
			vector<Point2f> mass_centre;//����

			center_mass(contours_d, mass_centre);//��������
			data_writing(mass_centre, long_axe, short_axe);//д���ı��ļ�
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



/*�׶���亯�����Ժϳɺ����ͨ��ʹ��*/
void fillHole(const Mat srcBw, Mat &dstBw)
{
	Size m_Size = srcBw.size();
	Mat Temp = Mat::zeros(m_Size.height + 2, m_Size.width + 2, srcBw.type());//��չͼ��
	srcBw.copyTo(Temp(Range(1, m_Size.height + 1), Range(1, m_Size.width + 1)));

	cv::floodFill(Temp, Point(0, 0), Scalar(255));

	Mat cutImg;//�ü���չ��ͼ��
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
	split(src, channels);//����ɫ��ͨ��

	imBlue = channels.at(0);
	imRed = channels.at(2);
	imGreen = channels.at(1);


	drawContours(imRed, contours, -1, CV_RGB(255, 255, 255), CV_FILLED, 8);//ͨ��Ⱦɫ
	//imshow("t", imRed);
	merge(channels, src);
}

int data_writing(vector<Point2f> mass_centre, vector<float> long_radius, vector<float> short_radius)//�ı��������
{
	ofstream in_file;
	in_file.open("out.txt");
	in_file << "���������\t" << long_radius.size() << "\n";
	in_file << "�������λ��\t" << "����\t" << "����\t" << "\n";

	for (int i = 0; i < long_radius.size(); i++)
	{
		in_file << mass_centre[i] << "\t" << long_radius[i] << "\t" << short_radius[i] << "\t" << "\n";
	}


	return 0;
}

void center_mass(vector<vector<Point> > contours, vector<Point2f> &mc)//�������ĵĺ���
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