/* bonepeg.cpp
 *
 * A program to stream video from a webcam connected to the BeagleBone to a Peggy 2.
 * With in-terminal preview, ASCII-cam style.
 * For now, needs 256-color capable terminal.
 *
 * Based on boneCV [1] by Derek Molloy, and his handy video [2].
 * Ascii character palettes from Paul Bourke [3].
 *
 * [1]: boneCV, https://github.com/derekmolloy/boneCV
 *
 * [2]: Molloy, D. [DerekMolloyDCU]. "Beaglebone: Video Capture and Image Processing on Embedded Linux using OpenCV [Video file]."
 *      25 May 2013. Youtube. 7 July 2013. <http://www.youtube.com/watch?v=8QouvYMfmQo>
 *
 * [3]: Bourke, Paul. "Character representation of greyscale images"
 *      Feb. 1997. Web. 8 July 2013.  <http://paulbourke.net/dataformats/asciiart/>
 *     
 */

#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <locale.h>
//#include <ncurses.h>

// Capture size from camera
#define WIDTH 800
#define HEIGHT 600

using namespace std;
using namespace cv;

// Declared functions
void printThumbnail(Mat thumb);
uint8_t grey82ansi(uint8_t grey8);
uint8_t grey82ansi(uint8_t grey8, uint8_t paletteSize);

// Globals
// 12pt Monaco blocks are 7x17, output pixels are using two spaces side-by-side for better aspect ratio.
int TERM_WIDTH = 55;
int TERM_HEIGHT = (TERM_WIDTH * 14) / 17;
Size peggySize = Size(TERM_WIDTH, TERM_HEIGHT);

/*
 * signal handler
 */
void my_handler(int s) {
    //endwin(); TODO ncurses
    printf("\x1B[?25h"); // show cursor

    // print message and restore prompt under video
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
        // cerr << "Can't set the specified locale! Check LANG, LC_CTYPE, LC_ALL." << endl; // any practical difference?
        fprintf(stderr, "Can't set the specified locale! "
                "Check LANG, LC_CTYPE, LC_ALL.\n");
        return 1;
    }

    // prints "VIDIOC_QUERYMENU: Invalid argument" on stderr a few times with this OpenCV, but works fine
    // Seems to be an issue with OpenCV querying device capabilities
    VideoCapture capture(0);
    capture.set(CV_CAP_PROP_FRAME_WIDTH, WIDTH);
    capture.set(CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);

    if (!capture.isOpened()) {
        cout << "Failed to connect to the camera." << endl;
    }

    // Matrices for captured frame, grey frame, and thumbnail
    Mat frame, cropped, grey, thumb;

    // Capture area is not the full frame, which may be 4:3 or 16:9 aspect, but the square in the middle.
    // Assumes that width > height
    Rect squareInCenter((WIDTH - HEIGHT) / 2, 0, HEIGHT, HEIGHT);
    
    
    // [2J   = Clear screen
    // [1;1H = [;H = put cursor at upper left (position 1,1)
    // [?25l = Hide cursor
    printf("\x1B[2J\x1B[;H\x1B[?25l");

    //initscr(); //TODO - ncurses
    //cbreak();
    //noecho();
    //start_color();
    //refresh();
    
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

    //endwin(); TODO ncurses

    return 0;
}

/* Prints the thumbnail to the terminal
 * assumes utf8 and 256 color support
 * TODO -- detect 256 color, utf8, provide fallbacks
 */
void printThumbnail(Mat thumb) {
    //char utf8boxes[] = "\xE2\x96\x88\xE2\x96\x88"; // two FULL BLOCK U+2588 in utf8
    // 16 characters arranged (roughly) in order of increasing density (brightness)
    //const char palette16[] = " .'^\":;!i?ZM#%@";
    //const std::string palette16 (" .'^\":;!i?ZM#%@");

    // These ramps suggested by Paul Bourke [3]
    //char palette10[] = " .:-=+*#%@";
    //char palette72[]= "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ";
    
    uint8_t rawGrey = 0, // 0-15
            ansiGrey = 0,
            rows = thumb.rows,
            cols = thumb.cols;

    // options
    bool mirror = true,
         utf8 = true,
         hicolor = true;

    // Move cursor to row 1, column 1
    printf("\x1B[;H");

    for(int i = 0; i < rows; i++ ) {
        for(int j = 0; j < cols; j++ ) {

            rawGrey = mirror ? thumb.at<uchar>(i, (cols - 1) - j) : thumb.at<uchar>(i, j);

            if (hicolor) {
                // Print two space characters with ansi grey background color
                // scaled across the 26 grey levels in the default ANSI 256 palette
                // extended ANSI colors are 0-255. 231 = #FFFFFF, 232 = #080808, 255 = #EEEEEE
                // To approximate peggy display:  grey82ansi(rawGrey, 16)
                ansiGrey = grey82ansi(rawGrey);
                printf("\x1B[48;5;%dm  ", ansiGrey);

                // TODO ncurses
                //mvprintw(0, j, "\x1B[48;5;%dm  ", ansiGrey);
                //mvprintw(0, j, "%c", palette16.at(rawGrey >> 4));
            }
        }

        printf("\n");
    }
    //refresh();
}

// Converts an 8-bit greyscale value to one of the 26 ANSI grey levels.
uint8_t grey82ansi(uint8_t grey8) {
    return grey82ansi(grey8, 26);
}

// Returns the best ANSI color code for the given 8-bit grey value, within the
// size of the palette.  26 ANSI grey levels are available, including black and white.
// color index 16 = black, 231 = white, 232 - 255 dark (#080808) to light (#EEEEEE) ramp 
//
// Wanring: the 256 colors in the palette can be redefined.
//
// @param grey8         an 8-bit grey value.  Gets scaled to 0-paletteSize.
// @param paletteSize   The number of greys in the palette.
uint8_t grey82ansi(uint8_t grey8, uint8_t paletteSize) {
    const uint8_t MAX_GREYS = 26, MIN_GREYS = 2;
    const uint8_t ANSIGREYS[MAX_GREYS] = { 16, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 
                                           244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 231 };
    uint8_t scaled = 0;

    // bounds check
    if (paletteSize > MAX_GREYS) { paletteSize = MAX_GREYS; }
    if (paletteSize < MIN_GREYS) { paletteSize = MIN_GREYS; }

    // if the paletteSize is a power of two [2,4,8,16] we can shift for scale
    // generic power of two test: paletteSize & (paletteSize - 1)) == 0)
    // Just hard-code the four supported
    switch(paletteSize) {
        case 16:
            scaled = grey8 >> 4; // 0-15
            break;
        case 8:
            scaled = grey8 >> 5; // 0-7
            break;
        case 4:
            scaled = grey8 >> 6; // 0-4
            break;
        case 2:
            scaled = grey8 >> 7; // 0-1, black or white
            break;
        default:
            scaled = (int)((grey8 * paletteSize) / (255 + 1)); // 0-(paletteSize-1)
    }

    // If using max greys in palette, the scaled value is equal to the index of the ansi grey color
    if (paletteSize == MAX_GREYS) {
        return ANSIGREYS[scaled];
    }

    int greyIdx = (scaled * (MAX_GREYS - 1)) / (paletteSize - 1);
    return ANSIGREYS[greyIdx]; 
}
