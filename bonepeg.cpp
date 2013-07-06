/* bonepeg.cpp
 *
 * Based on boneCV by Derek Molloy, School of Electronic Engineering, Dublin City University
 * www.derekmolloy.ie
 */

#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <locale.h>

// Capture size from camera
#define WIDTH 800
#define HEIGHT 600

using namespace std;
using namespace cv;

void my_handler(int s) {
    printf("%c[0mCaught signal %d\n", 0x1B, s);
    exit(1); 
}

int main(int argc, char** argv)
{
    // Trap CTRL-c
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    // set locale
     if (!setlocale(LC_CTYPE, "")) {
      fprintf(stderr, "Can't set the specified locale! "
                  "Check LANG, LC_CTYPE, LC_ALL.\n");
           return 1;
    }

    VideoCapture capture(0);
    capture.set(CV_CAP_PROP_FRAME_WIDTH, WIDTH);
    capture.set(CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);

    if(!capture.isOpened()){
	    cout << "Failed to connect to the camera." << endl;
    }

    // Matrices for captured frame, grey frame, and thumbnail
    Mat frame, grey, thumb;
    char palette[] =  " .'`,^:\";%$#@~-_+<>i!lI?/|()1{}[]rcvunxzjftLCJUYXZO0Qoahkbdpqwm*WMB8&%$#@";

    printf("%c[2J%c[;H", 0x1B, 0x1B);
   //  echo -e "\033[38;5;255m@\033[0m"
  while(1) {
    capture >> frame;
    if(frame.empty()){
		cout << "Failed to capture an image" << endl;
		return -1;
    }

    // Chop down width to make a square in the middle
    // For an 800x600 region, start crop left edge 100 pixels in.
    // For a 1280x720 region, start crop left edge 280 pixels in.
    int xOffset= (WIDTH - HEIGHT) / 2;
    Rect myROI(xOffset, 0, HEIGHT, HEIGHT);
    Mat cropped = frame(myROI);

    // Convert the region of interest into greyscale
    cvtColor(cropped, grey, CV_BGR2GRAY);

    // Resize greyscale image into peggy-sized thumbnail
    Size peggySize = Size(25,25);
    thumb = Mat(25, 25, CV_8U);
    resize(grey, thumb, peggySize, 0, 0, INTER_LANCZOS4);
    
    
    //cout << "Thumb = " << endl << thumb << endl << endl;
    

    printf("%c[1;1f", 0x1B);

    // char palette[] = " .,\"+=oOQE^*%$@#";
    int idx = 0;
    for(int i = 0; i < thumb.rows; i++ ) {
      for(int j = 0; j < thumb.cols; j++ ) {
        idx = thumb.at<uchar>(i,j) >> 4;	
	printf("%c[38;5;%dm\xE2\x96\x88\xE2\x96\x88", 0x1B, 232 + ((int) (1.43 * idx)));
      }
      printf("\n");
    }

  }
    return 0;
}
