#include "stdafx.h"
//
//#include <facedetect-dll.h>
//#include <opencv2/opencv.hpp>
//
//#ifdef _M_AMD64
//#pragma comment(lib, "libfacedetect-x64.lib")
//#ifdef _DEBUG
//#pragma comment(lib, "opencv_world341d.lib")
//#else
//#pragma comment(lib, "opencv_world341.lib")
//#endif
//#else
//#pragma comment(lib, "libfacedetect.lib")
//#endif
//
//#define DETECT_BUFFER_SIZE 0x20000
//using namespace cv;
//
//int tracking_test()
//{
//	VideoCapture cap(0); // open the default camera
//	if (!cap.isOpened())  // check if we succeeded
//		return -1;
//
//	
//	int * pResults = NULL;
//	kr::Array<kr::byte> buffer(DETECT_BUFFER_SIZE);
//
//	Mat gray;
//	Mat image;
//	namedWindow("Results", 1);
//	for (;;)
//	{
//		cap >> image; // get a new frame from camera
//		//cvtColor(frame, edges, COLOR_BGR2GRAY);
//		//GaussianBlur(edges, edges, Size(7, 7), 1.5, 1.5);
//		//Canny(edges, edges, 0, 30, 3);
//
//		cvtColor(image, gray, CV_BGR2GRAY);
//
//		//!!! The input image must be a gray one (single-channel)
//		//!!! DO NOT RELEASE pResults !!!
//
//		///////////////////////////////////////////
//		// frontal face detection / 68 landmark detection
//		// it's fast, but cannot detect side view faces
//		//////////////////////////////////////////
//		//pResults = facedetect_frontal(buffer.data(), (unsigned char*)(gray.ptr(0)), gray.cols, gray.rows, (int)gray.step,
//		//	1.2f, 2, 48, 0, 1);
//
//		///////////////////////////////////////////
//		// frontal face detection designed for video surveillance / 68 landmark detection
//		// it can detect faces with bad illumination.
//		//////////////////////////////////////////
//		pResults = facedetect_frontal_surveillance(buffer.data(), (unsigned char*)(gray.ptr(0)), gray.cols, gray.rows, (int)gray.step,
//			1.2f, 2, 48, 0, 1);
//
//		///////////////////////////////////////////
//		// multiview face detection / 68 landmark detection
//		// it can detect side view faces, but slower than facedetect_frontal().
//		//pResults = facedetect_multiview(buffer.data(), (unsigned char*)(gray.ptr(0)), gray.cols, gray.rows, (int)gray.step,
//		//	1.2f, 2, 48, 0, 1);
//
//		///////////////////////////////////////////
//		// reinforced multiview face detection / 68 landmark detection
//		// it can detect side view faces, better but slower than facedetect_multiview().
//		//pResults = facedetect_multiview_reinforce(buffer.data(), (unsigned char*)(gray.ptr(0)), gray.cols, gray.rows, (int)gray.step,
//		//	1.2f, 3, 48, 0, 1);
//
//		Mat result_frontal = image.clone();
//		//print the detection results
//		for (int i = 0; i < (pResults ? *pResults : 0); i++)
//		{
//			short * p = ((short*)(pResults + 1)) + 142 * i;
//			int x = p[0];
//			int y = p[1];
//			int w = p[2];
//			int h = p[3];
//			int neighbors = p[4];
//			int angle = p[5];
//
//			rectangle(result_frontal, Rect(x, y, w, h), Scalar(0, 255, 0), 2);
//
//			for (int j = 0; j < 68; j++)
//				circle(result_frontal, Point((int)p[6 + 2 * j], (int)p[6 + 2 * j + 1]), 1, Scalar(0, 255, 0));
//		}
//		imshow("Results", result_frontal);
//
//		if (waitKey(30) >= 0) break;
//	}
//	return 0;
//}