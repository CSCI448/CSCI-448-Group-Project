/*
 * ASMLibrary(Active Shape Model Library) -- A compact SDK for face alignment
 *
 * Copyright (c) 2008-2011, Yao Wei <njustyw@gmail.com>,
 * http://visionopen.com/members/yaowei/, All rights reserved.
 *
 * Refer to AUTHORS for acknowledgements.
 *
 * This software is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this software.  If not, see <http://www.gnu.org/licenses/>.
 */

/**@file 
A demo show how to do image alignment (face tracking) using active shape model 

Please cite the following or equivalent reference in any publicly available 
text that uses asmlibrary: 

YAO Wei. Research on Facial Expression Recognition and Synthesis.
<EM> Master Thesis, Department of Computer Science and Technology, 
Nanjing University</EM>, Feb 2009. \url http://code.google.com/p/asmlibrary

@version 6.0-2010-5-30
*/

#include <vector>
#include <string>
#include <iostream>

#include <cv.h>
#include <highgui.h>

#include "asmfitting.h"
#include "vjfacedetect.h"
#include "video_camera.h"

using namespace std;

static void print_version()
{
	printf("\n\n"
		   "/**************************************************************/\n"
		   "/*      ASMLibrary -- A compact SDK for face alignment        */\n"
		   "/*  Copyright (c) 2008-2011 by Yao Wei, all rights reserved.  */\n"
		   "/*                    Contact: njustyw@gmail.com              */\n"
		   "/**************************************************************/\n"
		   "\n\n");
}

static void usage_fit()
{
	printf("Usage: fit -m model_file -h cascade_file "
		"{-i image_file | -v video_file | -c camera_idx} -n n_iteration\n\n\n");
	exit(0);
}


int main(int argc, char *argv[])
{
	asmfitting fit_asm;
	char* model_name = NULL;
	char* cascade_name = NULL;
	char* filename = NULL;
	int use_camera = 0;
	int image_or_video = -1;
	int i;
	int n_iteration = 20;
	int camera_idx = 0;

	print_version();

	if(1 == argc)	usage_fit();
	for(i = 1; i < argc; i++)
	{
		if(argv[i][0] != '-') usage_fit();
		if(++i > argc)	usage_fit();
		switch(argv[i-1][1])
		{
		case 'm':
			model_name = argv[i];
			break;
		case 'h':
			cascade_name = argv[i];
			break;
		case 'i':
			if(image_or_video >= 0 || use_camera)
			{
				fprintf(stderr, "only process image/video/camera once\n");
				usage_fit();
			}
			filename = argv[i];
			image_or_video = 'i';
			break;
		case 'v':
			if(image_or_video >= 0 || use_camera)
			{
				fprintf(stderr, "only process image/video/camera once\n");
				usage_fit();
			}
			filename = argv[i];
			image_or_video = 'v';
			break;
		case 'c':
			if(image_or_video >= 0)
			{
				fprintf(stderr, "only process image/video/camera once\n");
				usage_fit();
			}
			use_camera = 1;
			camera_idx = atoi(argv[i]);
			break;
		case 'H':
			usage_fit();
			break;
		case 'n':
			n_iteration = atoi(argv[i]);
			break;
		default:
			fprintf(stderr, "unknown options\n");
			usage_fit();
		}
	}

	if(fit_asm.Read(model_name) == false)
		return -1;
	
	if(init_detect_cascade(cascade_name) == false)
		return -1;
	
	// case 1: process video, here we assume that the video contains only one face,
	// if not, we process with the most central face
	if(image_or_video == 'v')
	{
		int frame_count;
		asm_shape shape, detshape;
		bool flag = false;
		IplImage* image; 
		/* NOTE: the image must not be released, it will be dellocated automatically
		by the class asm_cam_or_avi*/
		int j;
	
		frame_count = open_video(filename);
		if(frame_count == -1)	return false;

		cvNamedWindow("ASM-Search",1);

		for(j = 0; j < frame_count; j ++)
		{
			double t = (double)cvGetTickCount();
			printf("Tracking frame %04i: ", j);
			
			image = read_from_video(j);
			
			if(j == 0 || flag == false)
			{
				//Firstly, we detect face by using Viola_jones haarlike-detector
				flag = detect_one_face(detshape, image);
				
				//Secondly, we initialize shape from the detected box
				if(flag) 
				{	
					InitShapeFromDetBox(shape, detshape, fit_asm.GetMappingDetShape(), fit_asm.GetMeanFaceWidth());
				}
				else goto show;
			}
			
			//Thirdly, we do image alignment 
			flag = fit_asm.ASMSeqSearch(shape, image, j, true, n_iteration);
			
			//If success, we draw and show its result
			if(flag) fit_asm.Draw(image, shape);
show:
			cvShowImage("ASM-Search", image);
			cvWaitKey(1);
			
			t = ((double)cvGetTickCount() -  t )/  (cvGetTickFrequency()*1000.);
			printf("ASM fitting time cost: %.2f millisec\n", t);
		}

		close_video();
	}
	// case 2: process image, we can process multi-person image alignment
	// also you can process single face alignment by coding like this
	// asm_shape detshape, shape;	
	// bool flag = face_detect.DetectCentralFace(detshape, image);
	// if(flag) asm_common::InitShapeFromDetBox(shape, detshape, 
	//		fit_asm.GetMappingDetShape(), fit_asm.GetMeanFaceWidth());
	// fit_asm.Fitting(shape, image, n_iteration);
	// shape.Write(stdout); //print result
	// for(int l = 0; l < shape.NPoints(); l++)
	//		printf("(%g, %g) ", shape[i].x, shape[i].y);
	else if(image_or_video == 'i')
	{
		IplImage * image = cvLoadImage(filename, 1);
		if(image == 0)
		{
			fprintf(stderr, "Can not Open image %s\n", filename);
			exit(0);
		}

		double t = (double)cvGetTickCount();
		int nFaces;
		asm_shape *shapes = NULL, *detshapes = NULL;
		
		// step 1: detect face
		bool flag =detect_all_faces(&detshapes, nFaces, image);
		
		// step 2: initialize shape from detect box
		if(flag)
		{
			shapes = new asm_shape[nFaces];
			for(int i = 0; i < nFaces; i++)
			{
				InitShapeFromDetBox(shapes[i], detshapes[i], fit_asm.GetMappingDetShape(), fit_asm.GetMeanFaceWidth());
			}
		}
		else 
		{
			fprintf(stderr, "This image doesnot contain any faces!\n");
			exit(0);
		}
		
		// step 3: image alignment fitting
		fit_asm.Fitting2(shapes, nFaces, image, n_iteration);
		
		t = ((double)cvGetTickCount() -  t )/  (cvGetTickFrequency()*1000.);
			printf("ASM fitting time cost: %.2f millisec\n", t);
			
		// step 4: draw and show result in GUI
		for(int i = 0; i < nFaces; i++)
		{
			fit_asm.Draw(image, shapes[i]);
		}
		
		cvSaveImage("result.jpg", image);
		cvNamedWindow("Fitting", 1);
		cvShowImage("Fitting", image);	
		cvWaitKey(0);			
		cvReleaseImage(&image);

		// step 5: free resource
		delete[] shapes;
		free_shape_memeory(&detshapes);
	}
	// case 3: process camera
	else if(use_camera)
	{
		asm_shape shape, detshape;
		bool flag = false;
		IplImage* image;
		IplImage* imageRaw;
		IplImage* im2;
		IplImage* lEye;
		IplImage* rEye;
		IplImage* mouth;
		int j = 0;
		int pointNum = 0;
		
		if(open_camera(camera_idx) == false)
		{
			fprintf(stderr, "Can not open camera [%d]\n", camera_idx);
			exit(0);
		}

		cvNamedWindow("ASM-Search",1);
//		cvNamedWindow("Point",2);
		cvNamedWindow("Left_Eye",2);
		cvNamedWindow("Right_Eye",2);
		cvNamedWindow("Mouth",2);

		while(1)
		{
			// NOTE: when the parameter is set 1, we can read from camera
			image = read_from_camera();
			imageRaw = cvCreateImage(cvGetSize(image),image->depth,image->nChannels);
			cvCopy(image,imageRaw,NULL);
			
			//if(flag == false)
			//{
				//Firstly, we detect face by using Viola_jones haarlike-detector
				flag = detect_one_face(detshape, image);
				
				//Secondly, we initialize shape from the detected box
				if(flag)
				{
					InitShapeFromDetBox(shape, detshape, fit_asm.GetMappingDetShape(), fit_asm.GetMeanFaceWidth());
					j ++;
				}
			//}
			
			//Thirdly, we do image alignment 
			flag = fit_asm.ASMSeqSearch(shape, image, j, true, n_iteration);
			
			//If success, we draw and show its result
			fit_asm.Draw(image, shape);

			//left eye
			double lety = shape[33].y-20;
			double leby = shape[35].y+20;
			double lerx = shape[34].x-20;
			double lelx = shape[32].x+20;
			double leh = leby-lety;
			double lew = lelx - lerx;
			cvSetImageROI(imageRaw,cvRect(lerx,lety,lew,leh));
			lEye = cvCreateImage(cvGetSize(imageRaw),imageRaw->depth,imageRaw->nChannels);
			cvCopy(imageRaw,lEye,NULL);
			cvResetImageROI(imageRaw);
			cvShowImage("Left_Eye",lEye);
			cvResizeWindow("Left_Eye",2*lew,2*leh);

			//right eye
			double rety = shape[28].y-20;
			double reby = shape[30].y+20;
			double rerx = shape[27].x-20;
			double relx = shape[29].x+20;
			double reh = reby-rety;
			double rew = relx - rerx;
			cvSetImageROI(imageRaw,cvRect(rerx,rety,rew,reh));
			rEye = cvCreateImage(cvGetSize(imageRaw),imageRaw->depth,imageRaw->nChannels);
			cvCopy(imageRaw,rEye,NULL);
			cvResetImageROI(imageRaw);
			cvShowImage("Right_Eye",rEye);
			cvResizeWindow("Right_Eye",2*rew,2*reh);

			//mouth
			double mby = shape[57].y + 20;
			double mlcx = shape[54].x + 20;
			double mrcx = shape[48].x - 20;

			double mrcy = shape[48].y - 20;
			double mlcy = shape[54].y - 20;
			double mtly = shape[52].y - 20;
			double mtry = shape[50].y - 20;
			double top1 = min(mrcy,mlcy);
			double top2 = min(mtly,mtry);
			double top = min(top1,top2);
			double mw = mlcx - mrcx;
			double mh = mby - top;

			cvSetImageROI(imageRaw,cvRect(mrcx,top,mw,mh));
			mouth = cvCreateImage(cvGetSize(imageRaw),imageRaw->depth,imageRaw->nChannels);
			cvCopy(imageRaw,mouth,NULL);
			cvResetImageROI(imageRaw);
			cvShowImage("Mouth",mouth);
			cvResizeWindow("Mouth",2*mw,2*mh);


show2:
//			cvShowImage("Point",im2);
			cvShowImage("ASM-Search", image);
			if ( (cvWaitKey(1) & 255) == 27 ) {
//				pointNum++;
//				cout<<pointNum<<endl;
				break;
			}
		}

		close_camera();
	}

    return 0;
}



