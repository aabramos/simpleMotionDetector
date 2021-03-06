#include <iostream> 
#include <time.h> 
#include <windows.h>
#include <mmsystem.h>
#include "cv.h" 
#include "highgui.h"

// Library for sounds
#pragma comment(lib,"Winmm.lib")

/* 
 * Simple Motion Detector
 * By Adriano A. B. Ramos, Rafael Mansoldo
 */ 
  
int main(int argc, char** argv) 
{  
	int k;
	int alarme = 0;
	double timestamp;

	// OpenCv Declarations
	IplImage* nextFrame = 0;
	IplImage* diff = 0;
	IplImage *mhi = 0;
	IplImage* segmask = 0;
	IplImage* mask = 0;
	IplImage* blueMotion = 0; 
	IplImage* temp = 0; 
	IplImage* temp2 = 0; 
	CvMemStorage* storage = 0;
	CvRect comp_rect; 
	CvSeq* seq;

    CvCapture* capture = 0;
	IplImage* frame = 0;
	
    if(argc == 1)
        capture = cvCaptureFromCAM(0);
    else if(argc == 2)
        capture = cvCaptureFromAVI(argv[1]);

    if(!capture)
	{
        fprintf(stderr,"Capture settings could not be initialized.\n");
        return -1;
    }

	cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH,  640.00);
	cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, 480.00);

	printf( "Simple Motion Detector\n"
			"Commands: \n"
			"\ta - Activate alarm\n"
			"\td - Deactivate alarm\n"
            "\tESC - Exit\n"
			);

	frame = cvQueryFrame(capture); 
	nextFrame = cvQueryFrame(capture); 
  
	diff = cvCreateImage(cvSize(frame->width,frame->height), IPL_DEPTH_8U, 1 ); 
	mhi = cvCreateImage(cvSize(frame->width,frame->height), IPL_DEPTH_32F, 1 ); 
	segmask = cvCreateImage(cvSize(frame->width,frame->height), IPL_DEPTH_32F, 1 ); 
	mask = cvCreateImage(cvSize(frame->width,frame->height), IPL_DEPTH_8U, 1 ); 
	temp = cvCreateImage(cvSize(frame->width,frame->height), IPL_DEPTH_8U, 1); 
	temp2 = cvCreateImage(cvSize(frame->width,frame->height), IPL_DEPTH_8U, 1); 
	blueMotion = cvCloneImage(frame); 

	cvZero(mhi); 
 	cvCvtPixToPlane(frame, temp, NULL, NULL, NULL); 

	if(!storage)
        storage = cvCreateMemStorage(0);
	else
        cvClearMemStorage(storage);

	CvRect lastComp = cvRect(0,0,frame->width, frame->height);

	for(;;) 
	{ 
		if(nextFrame != NULL) 
		{ 
			// Motion Detection Rectangle Coordinates for 640x480 Resolution Capture
			CvRect Roi = cvRect(frame->width/2.5,frame->height/2.5,150,150); 

			// ROI - OpenCV Region of Interest
			cvSetImageROI(frame, Roi);
			cvSetImageROI(nextFrame, Roi); 
			cvSetImageROI(temp, Roi); 
			cvSetImageROI(temp2, Roi); 			

			cvCvtPixToPlane(nextFrame, temp2, NULL, NULL, NULL); 

			timestamp = (double)clock()/CLOCKS_PER_SEC; 
			cvSetImageROI(diff, Roi); 
			cvAbsDiff(temp, temp2, diff); // calculate frame difference
			cvSetImageROI(mhi, Roi); 
			cvThreshold(diff, diff, 150, 1, CV_THRESH_BINARY); 
			cvUpdateMotionHistory( diff, mhi, timestamp, 1); // update MHI 

			// convert MHI to blue 8u 
			cvSetImageROI(mask, Roi); 
			cvCvtScale(mhi, mask, 255./1,(2 - timestamp)*255./1 ); 
			blueMotion = cvCloneImage(frame); 
			cvSetImageROI(blueMotion, Roi);
			cvSetImageROI(segmask, Roi); 
			cvCvtPlaneToPix(mask, 0, 0, 0, blueMotion); 
			seq = cvSegmentMotion(mhi, segmask, storage, timestamp, 0.5); 

			for(int i = -1; i < seq->total; i++)  
			{ 
				if( i >= 0 )
				{ 
					comp_rect = ((CvConnectedComp*)cvGetSeqElem( seq, i ))->rect; 

					if( comp_rect.width + comp_rect.height < 0 ) // Sensibility
						continue;

					CvScalar color = CV_RGB(255,255,0); 
                    cvRectangle(frame, cvPoint(comp_rect.x - 18, comp_rect.y-18),   
                    cvPoint(comp_rect.x + 18, comp_rect.y+18), color, 3);   
                    cvRectangle(blueMotion, cvPoint(comp_rect.x-18, comp_rect.y-18),   
                    cvPoint(comp_rect.x + 18, comp_rect.y+18), color, 3); 

					// Continuous alarm
					if (!alarme)
					{
						if((lastComp.y > comp_rect.y+30) || (lastComp.y < comp_rect.y-30) || (lastComp.x > comp_rect.x+27) || (lastComp.x < comp_rect.x-27)) 
						{ 	

							PlaySound(L"buzz.wav", NULL, SND_ASYNC | SND_LOOP);
							printf( "\n- Motion Detected!");
							alarme = 1;
						}
					} 
					lastComp = comp_rect; 
				} 
			} 

			cvResetImageROI(mhi); 
			cvResetImageROI(blueMotion); 
			cvResetImageROI(frame); 
			cvResetImageROI(temp2); 
			cvResetImageROI(nextFrame); 
			cvResetImageROI(diff); 
			cvResetImageROI(mask); 
			cvShowImage("Simple Motion Detector",blueMotion); 
		} 		
		k = cvWaitKey(10); 

		switch (k) 
		{
			case 27:  // Escape
				cvReleaseCapture(&capture); 
				cvDestroyWindow("Simple Motion Detector"); 
				cvReleaseImage(&temp); 
				cvReleaseImage(&temp2); 
				cvReleaseImage(&blueMotion); 
				cvReleaseImage(&mhi); 
				exit(0);
			case 'd':  // Deactivate alarm
				PlaySound(NULL, NULL, NULL);
				alarme = 1;
				printf( "\n- Alarm disabled");
				break;
			case 'a':  // Activate alarm
				if (alarme)
					alarme = 0;
				printf( "\n- Alarm enabled");
				break;
		}

		// Capture next frame
		frame = nextFrame; 
		nextFrame = cvQueryFrame(capture); 
	} 
}
