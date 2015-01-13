 #include "opencv2/opencv.hpp"
 #include "opencv2/highgui/highgui.hpp"
 #include "opencv2/imgproc/imgproc.hpp"

 #include <iostream>
 #include <stdio.h>
 #include <stdlib.h>
 #include <algorithm>
 #include <vector>

 #define WINVER 0x0500
 #include <windows.h>
 
 using namespace cv;
 using namespace std;

 class MouseSimulator
{
      INPUT Input;
      double fScreenWidth,fScreenHeight,widthRatio,heightRatio;
      double fx,fy;
      public:
      MouseSimulator()
      {
             fScreenWidth    = ::GetSystemMetrics( SM_CXSCREEN )-1; 
             fScreenHeight  = ::GetSystemMetrics( SM_CYSCREEN )-1; 
             widthRatio = fScreenWidth/(640-160);//25% of 640
             heightRatio = fScreenHeight/(480-120);//25% of 480
      }
      void MoveCursor(float x,float y)
      {
           fx=x*widthRatio;
           fy=y*heightRatio;
           ::ZeroMemory(&Input,sizeof(INPUT));
           Input.type      = INPUT_MOUSE;
           Input.mi.dwFlags  = MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE;
           Input.mi.dx = fx*(65535.0f/fScreenWidth);
           Input.mi.dy = fy*(65535.0f/fScreenHeight);
           ::SendInput(1,&Input,sizeof(INPUT));
      }
      void DoubleClick(float x,float y)
      {
          LeftClick(x,y);
          LeftClick(x,y);
      }
      void LeftClick(float x,float y)
      {
           MoveCursor(x,y);
           LClick(x,y);
      }
      void RightClick(float x,float y)
      {
           MoveCursor(x,y);
           RClick(x,y);
      }
      private:     
      void RClick(float x,float y)
      {
           fx=x*widthRatio;
           fy=y*heightRatio;

           //right up
           ::ZeroMemory(&Input,sizeof(INPUT));
           Input.type=INPUT_MOUSE;
           Input.mi.dwFlags  = MOUSEEVENTF_RIGHTDOWN;
           Input.mi.dx = fx*(65535.0f/fScreenWidth);
           Input.mi.dy = fy*(65535.0f/fScreenHeight);

           ::SendInput(1,&Input,sizeof(INPUT));

           // right up
           ::ZeroMemory(&Input,sizeof(INPUT));
           Input.type      = INPUT_MOUSE;
           Input.mi.dwFlags  = MOUSEEVENTF_RIGHTUP;
           Input.mi.dx = fx*(65535.0f/fScreenWidth);
           Input.mi.dy = fy*(65535.0f/fScreenHeight);

           ::SendInput(1,&Input,sizeof(INPUT));
     }
     void LClick(float x,float y)
     {
           fx=x*widthRatio;
           fy=y*heightRatio;

           //left up
           ::ZeroMemory(&Input,sizeof(INPUT));
           Input.type=INPUT_MOUSE;
           Input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;
           Input.mi.dx = fx*(65535.0f/fScreenWidth);
           Input.mi.dy = fy*(65535.0f/fScreenHeight);
           ::SendInput(1,&Input,sizeof(INPUT));

           // right up
           ::ZeroMemory(&Input,sizeof(INPUT));
           Input.type      = INPUT_MOUSE;
           Input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;
           Input.mi.dx = fx*(65535.0f/fScreenWidth);
           Input.mi.dy = fy*(65535.0f/fScreenHeight);

           ::SendInput(1,&Input,sizeof(INPUT));
     }
     
};

 void perform_Gesture_Action(int cur_ges_num);
 int findBiggestContour(vector<vector<Point> >);
 Mat myBackgroundSubtractor(Mat background,Mat current);

 int gesture_number=0;
 int prev_ges_num=0;
 int ges_num_times=0;
 int ptx=100,pty=100;
 Point2f hulltop,hullbottom;
 
 MouseSimulator mymouse;
 
 int main(int argc, char *argv[])
{
    Mat bgimg;
	Mat bg_avg;
	Mat bg_hsv;
    Mat fore,fore_copy;
	Mat hsv;
	Mat bw,diffimg,back_static;
	Mat bgimg_clone;
	Mat h_image,s_image,v_image;
	Mat h_gray,s_gray,v_gray;
	std::vector<cv::Mat> hsv_channels;

	//Mat canny_output;
	vector<pair<Point,double> > palm_centers;
    VideoCapture cap(0);
 
    std::vector<std::vector<cv::Point> > contours;
	for(int i=0;i<50;i++)
	{
 		cap>>bgimg;
	}
	medianBlur ( bgimg, bgimg, 3 );
	erode(bgimg,bgimg,cv::Mat());
	dilate(bgimg,bgimg,cv::Mat());
	cvtColor(bgimg,bg_avg,CV_BGR2HSV);

	for(int i=0;i<50;i++)
	{
		cap>>bgimg;
		medianBlur ( bgimg, bgimg, 3 );
		erode(bgimg,bgimg,cv::Mat());
		dilate(bgimg,bgimg,cv::Mat());
		cvtColor(bgimg,bg_hsv,CV_BGR2HSV);
		
		addWeighted(bg_avg,0.8,bg_hsv,0.2,0,bg_avg);
	}
	Mat drawing = Mat::zeros( bgimg.size(), CV_8UC1 );

	for(;;)
    {
	    cap >> fore;
		fore_copy=fore.clone();
		medianBlur ( fore, fore, 3 );
		cv::erode(fore,fore,cv::Mat());
        cv::dilate(fore,fore,cv::Mat());
		cvtColor(fore,hsv,CV_BGR2HSV);
	
		absdiff(bg_avg,hsv,diffimg);
		cv::split(diffimg, hsv_channels);
		v_image = hsv_channels[2];
		threshold(v_image,v_gray,25,255,THRESH_BINARY);
		bw=v_gray;
		medianBlur(bw,bw,9);
		
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		findContours( bw, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
		int biggest_contour_index = findBiggestContour(contours);
		
		if(biggest_contour_index!=-1)
		{
			drawContours( drawing, contours, biggest_contour_index, Scalar(255), -1, 8, hierarchy, 0, Point() );
			drawContours( fore_copy, contours, biggest_contour_index, Scalar(255), 5, 8, hierarchy, 0, Point() );
			
			//convex hull processing
			vector<vector<Point> >hull(contours.size() );
			Scalar color=Scalar(0,0,255);
			convexHull( Mat(contours[biggest_contour_index]), hull[biggest_contour_index], false ); 
			drawContours( fore_copy, hull, biggest_contour_index, color, 5, 8, vector<Vec4i>(), 0, Point() );
			
			//finding convexity defects
			vector<vector<int> > hull_i(contours.size() );
			vector<vector<Vec4i> > defects(contours.size());
			convexHull( Mat(contours[biggest_contour_index]), hull_i[biggest_contour_index], false );
			convexityDefects(Mat(contours[biggest_contour_index]),hull_i[biggest_contour_index],defects[biggest_contour_index]);
			
			//finding top and bottom point of hull
			hulltop.x=0;hulltop.y=480;
			hullbottom.x=0;hullbottom.y=0;
			vector<Point>::iterator p= hull[biggest_contour_index].begin();
			while(p!=hull[biggest_contour_index].end())
			{
				Point& p1=(*p);
				if(hulltop.y > p1.y)
				{
					hulltop.x=p1.x;
					hulltop.y=p1.y;
				}
				if(hullbottom.y<p1.y)
				{
					hullbottom.x=p1.x;
					hullbottom.y=p1.y;
				}
				p++;
			}
			int val=hullbottom.y-hulltop.y;
			val=(int)(val*0.75);
			val=hulltop.y+val;
			//finding defect number and drawing defects
			gesture_number=0;
			vector<Vec4i>::iterator d=defects[biggest_contour_index].begin();
			while( d!=defects[biggest_contour_index].end() )
			{
				Vec4i& v=(*d);
				int startidx=v[0]; 
				Point ptStart( contours[biggest_contour_index][startidx] );
				int endidx=v[1]; 
				Point ptEnd( contours[biggest_contour_index][endidx] );
				int faridx=v[2]; 
				Point ptFar( contours[biggest_contour_index][faridx] );
				float depth = v[3] / 256.0f;
				if( depth>50 && ptFar.y<val )
				{
						circle( fore_copy, ptFar,   5, Scalar(0,0,255), 5 );
						gesture_number++;
				}
				d++;
			}
			gesture_number++;
			//cout<<"\nGesture Number : "<<gesture_number;
			if(gesture_number==1 || gesture_number==2)
			{				
				circle(fore_copy,hulltop,3,Scalar(0,255,0),3);
				ptx=hulltop.x;pty=hulltop.y;
			}
			hull.clear();
			hull_i.clear();
			defects.clear();
		}
		
     	perform_Gesture_Action(gesture_number);
		imshow("frame",fore_copy);
		contours.clear();
		hierarchy.clear();
        if(cv::waitKey(10) >= 0) 
			break;
	}
    return 0;
}

void perform_Gesture_Action(int cur_ges_num)
{
	if(prev_ges_num==cur_ges_num)
		ges_num_times++;
	prev_ges_num=cur_ges_num; 
	if(ges_num_times==4)
	{
		switch(cur_ges_num)
		{
			case 3:
				//cout<<"\n ptx pty:"<<ptx<<" "<<pty;
				mymouse.LeftClick(ptx,pty);
				break;
			case 4:
				//cout<<"\n ptx pty:"<<ptx<<" "<<pty;
				mymouse.DoubleClick(ptx,pty);
				break;
			case 5:
				//cout<<"\n ptx pty:"<<ptx<<" "<<pty;
				mymouse.RightClick(ptx,pty);
				break;
			default:
				mymouse.MoveCursor(ptx,pty);
		}
		ges_num_times=0;
	}
	else if((cur_ges_num==1 || cur_ges_num==2) && ges_num_times==2)
	{
		mymouse.MoveCursor(ptx,pty);
		ges_num_times=0;
	}
}

int findBiggestContour(vector<vector<Point> > contours)
{
    int indexOfBiggestContour = -1;
    int sizeOfBiggestContour = 0;
    for (int i = 0; i < contours.size(); i++)
	{
        if(contours[i].size() > sizeOfBiggestContour)
		{
            sizeOfBiggestContour = contours[i].size();
            indexOfBiggestContour = i;
        }
    }
    return indexOfBiggestContour;
}

bool approx(unsigned char a, unsigned char b)
{
	if( b<=a+10 && b>=a-10)
		return true;
	else return false;
}

Mat myBackgroundSubtractor(Mat bg,Mat cur)
{
	//for bgr images
	
	Mat diff(cur.rows,cur.cols,cur.type());
	
	cout<<"\nVALUES:  cur:"<<cur.rows<<" "<<cur.cols;
	for (int i = 0; i < cur.rows; i++)
	{
		for (int j = 0; j < cur.cols; j++)
		{
			//cout<<"\n i j: "<<i<<" "<<j;
			if( approx(bg.data[bg.channels()*(bg.cols*i+j)+0],cur.data[cur.channels()*(cur.cols*i+j)+0]) &&
				approx(bg.data[bg.channels()*(bg.cols*i+j)+1],cur.data[cur.channels()*(cur.cols*i+j)+1]) &&
				approx(bg.data[bg.channels()*(bg.cols*i+j)+2],cur.data[cur.channels()*(cur.cols*i+j)+2])
				)
			{
				diff.data[cur.channels()*(cur.cols*i+j)+0]=0;
				diff.data[cur.channels()*(cur.cols*i+j)+1]=0;
				diff.data[cur.channels()*(cur.cols*i+j)+2]=0;
			}
			else 
			{
				diff.data[cur.channels()*(cur.cols*i+j)+0]=cur.data[cur.channels()*(cur.cols*i+j)+0];
				diff.data[cur.channels()*(cur.cols*i+j)+1]=cur.data[cur.channels()*(cur.cols*i+j)+1];
				diff.data[cur.channels()*(cur.cols*i+j)+2]=cur.data[cur.channels()*(cur.cols*i+j)+2];
			}
		}
	}
	//absdiff(bg,cur,diff);
	
	//imshow("diff",diff);
	//waitKey(10);
	return diff;

	//for gray scale images
	/*
	Mat bg_gray,cur_gray;
	cvtColor(bg,bg_gray,CV_BGR2GRAY);
	cvtColor(cur,cur_gray,CV_BGR2GRAY);
	Mat diff(cur.rows,cur.cols,cur.type());

	for(int i=0;i<cur_gray.rows;i++)
		for(int j=0;j<cur_gray.cols;j++)
		{
			 if(((bg_gray.at<uchar>(i,j)-30)<= cur_gray.at<uchar>(i,j)) && ((bg_gray.at<uchar>(i,j)+30)>= cur_gray.at<uchar>(i,j)))
			 {
				diff.data[cur.channels()*(cur.cols*i+j)+0]=0;
				diff.data[cur.channels()*(cur.cols*i+j)+1]=0;
				diff.data[cur.channels()*(cur.cols*i+j)+2]=0;
			 }
			 else 
			 {
				diff.data[cur.channels()*(cur.cols*i+j)+0]=cur.data[cur.channels()*(cur.cols*i+j)+0];
				diff.data[cur.channels()*(cur.cols*i+j)+1]=cur.data[cur.channels()*(cur.cols*i+j)+1];
				diff.data[cur.channels()*(cur.cols*i+j)+2]=cur.data[cur.channels()*(cur.cols*i+j)+2];
			 }
		}
	//cout<<"\n after for loop";
	imshow("diff",diff);
	return diff;
	*/
}