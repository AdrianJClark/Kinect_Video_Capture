#include <XnOS.h>
#include <XnCppWrapper.h>

//OpenCV
#include <opencv\cv.h>
#include <opencv\highgui.h>

bool recording=false;
bool running = true;

using namespace xn;

#define SAMPLE_XML_PATH "Data/SamplesConfig.xml" // OpenNI config file

void main() {
	// OpenNI Global
	Context niContext;
	DepthGenerator niDepth;
	ImageGenerator niImage;

	CvVideoWriter *cVW, *dVW;

	EnumerationErrors errors;
	switch (XnStatus rc = niContext.InitFromXmlFile(SAMPLE_XML_PATH, &errors)) {
		case XN_STATUS_OK:
			break;
		case XN_STATUS_NO_NODE_PRESENT:
			XnChar strError[1024];	errors.ToString(strError, 1024);
			printf("%s\n", strError);
			return;
		default:
			printf("Open failed: %s\n", xnGetStatusString(rc));
			return;
	}

	niContext.FindExistingNode(XN_NODE_TYPE_DEPTH, niDepth);
	niContext.FindExistingNode(XN_NODE_TYPE_IMAGE, niImage);

	niDepth.GetMirrorCap().SetMirror(false);
	//Align the depth image and colourImage
	niDepth.GetAlternativeViewPointCap().SetViewPoint(niImage);
	
	while (running) {
		if (XnStatus rc = niContext.WaitAnyUpdateAll() != XN_STATUS_OK) {
			printf("Read failed: %s\n", xnGetStatusString(rc));
			return;
		}

		// Update MetaData containers
		DepthMetaData niDepthMD; ImageMetaData niImageMD;
		niDepth.GetMetaData(niDepthMD); niImage.GetMetaData(niImageMD);

		// Extract Colour Image
		IplImage *colourIm = cvCreateImage(cvSize(niImageMD.XRes(), niImageMD.YRes()), IPL_DEPTH_8U, 3);
		memcpy(colourIm->imageData, niImageMD.Data(), colourIm->imageSize); cvCvtColor(colourIm, colourIm, CV_RGB2BGR);
		cvFlip(colourIm, colourIm, 1);

		// Extract Depth Image
		IplImage *depthIm = cvCreateImage(cvSize(niDepthMD.XRes(), niDepthMD.YRes()), IPL_DEPTH_16U, 1);
		memcpy(depthIm->imageData, niDepthMD.Data(), depthIm->imageSize);

		IplImage *depthView = cvCreateImage(cvGetSize(depthIm), IPL_DEPTH_8U, 1);
		cvConvertScale(depthIm, depthView, .1);

		
		cvShowImage("Colour Image", colourIm);
		cvShowImage("Depth Image", depthView);

		if (recording) {
			cvWriteFrame(cVW, colourIm);
			cvWriteFrame(dVW, depthIm);
		}

		switch(cvWaitKey(1)) {
			case 27:
				running = false;
				break;
			case ' ':
				recording = !recording;
				if (recording) {
					cVW = cvCreateAVIWriter("kinect-colour.avi", -1, 30, cvSize(640,480), 1);
					dVW = cvCreateAVIWriter("kinect-depth.avi", -1, 30, cvSize(640,480), 0);
				} else {
					cvReleaseVideoWriter(&cVW); cvReleaseVideoWriter(&dVW);
				}
				break;
		}

		cvReleaseImage(&depthView);
		cvReleaseImage(&depthIm);
		cvReleaseImage(&colourIm);
	}

	
	if (recording) {
		cvReleaseVideoWriter(&cVW); cvReleaseVideoWriter(&dVW);
	}
}
