/* bonepeg.cpp
 *
 * Based on boneCV[1] by Derek Molloy, and his handy video [2].
 * Ascii character palettes from Paul Bourke [3].
 *
 * 1 = boneCV, https://github.com/derekmolloy/boneCV
 *
 * 2 = Molloy, D. [DerekMolloyDCU]. "Beaglebone: Video Capture and Image Processing on Embedded Linux using OpenCV [Video file]."
 *     25 May 2013. Youtube. 7 July 2013. <http://www.youtube.com/watch?v=8QouvYMfmQo>
 *
 * 3 = Bourke, Paul. "Character representation of greyscale images"
 *     Feb. 1997. Web. 8 July 2013.  <http://paulbourke.net/dataformats/asciiart/>
 *     
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

// Declared functions
void printThumbnail(Mat thumb);


// Globals
Size peggySize = Size(25,25);

void my_handler(int s) {
    printf("\x1B[0m\x1B[%d;1H\nCaught signal %d\n", peggySize.height, s);
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
    Mat frame, cropped, grey, thumb;
    Rect squareInCenter((WIDTH - HEIGHT) / 2, 0, HEIGHT, HEIGHT);
    
    // 16 characters arranged (roughly) in order of increasing density (brightness)
    char palette16[] = " .'^\":;!i?ZM#%@";

	// These ramps suggested by Paul Bourke [3]
    //char palette10[] = " .:-=+*#%@";
	//char palette72[]= "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ";
    
    // [2J = Clear screen, [1;1H = [;H = put cursor at upper left (position 1,1)
    printf("\x1B[2J\x1B[;H");
    
    /*
     * Main webcam capture loop
     */
    for(;;)
    {
        // capture frame from camera or bail
        capture >> frame;

        if (frame.empty()) {
            cout << "Failed to capture an image" << endl;
            return -1;
        }

        // Square center crop, chop width to match height, equals bits from both sides
        cropped = frame(squareInCenter);

        // Convert the region of interest into greyscale
        cvtColor(cropped, grey, CV_BGR2GRAY);

        // Resize greyscale image into peggy-sized thumbnail, 8-bit unsigned matrix of grey level
        thumb = Mat(peggySize.width, peggySize.height, CV_8U);
        resize(grey, thumb, peggySize, 0, 0, INTER_LANCZOS4);

        //cout << "Thumb = " << endl << thumb << endl << endl; // debug
        printThumbnail(thumb);


    }//capture loop

    return 0;
}

// Prints the thumbnail to the terminal
// assumes utf8 and 256 color support
// TODO -- detect 256 color, utf8, provide fallbacks
void printThumbnail(Mat thumb) {
	int fourBitGrey = 0;
    int ansiGrey = 0;
	char utf8boxes[] = "\xE2\x96\x88\xE2\x96\x88";


    // Move cursor to row 1, column 1
    printf("\x1B[;H");

	for(int i = 0; i < thumb.rows; i++ ) {
		for(int j = 0; j < thumb.cols; j++ ) {
			fourBitGrey = thumb.at<uchar>(i,j) >> 4; // 0-15

			// extended ANSI colors are 0-255.  Black starts at 232, white at 255.
			ansiGrey = 232 + ((int) (1.43 * fourBitGrey));
			printf("\x1B[38;5;%dm%s", ansiGrey, utf8boxes);
		}

		printf("\n");
	}
}
