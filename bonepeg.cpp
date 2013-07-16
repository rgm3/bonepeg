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
#include <ncurses.h>

// Capture size from camera
#define WIDTH 800
#define HEIGHT 600

//using namespace std;
using namespace cv;

// Declared functions
void printThumbnail(Mat thumb);
uint8_t grey82ansi(uint8_t grey8);
uint8_t grey82ansi(uint8_t grey8, uint8_t paletteSize);
int mapVal(int value, int fromLow, int fromHigh, int toLow, int toHigh);

// Globals
Size peggySize = Size(25, 25);

/*
 * signal handler
 */
void my_handler(int s) {
    endwin();
    printf("Caught signal %d\n", s);
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
        // std::cerr << "Can't set the specified locale! Check LANG, LC_CTYPE, LC_ALL." << std::endl; // any practical difference?
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
        std::cout << "Failed to connect to the camera." << std::endl;
        return 2;
    }

    // Matrices for captured frame, grey frame, and thumbnail
    Mat frame, cropped, grey, thumb;
    Rect squareInCenter((WIDTH - HEIGHT) / 2, 0, HEIGHT, HEIGHT);
    
    // ncurses setup 
    initscr();
    cbreak();
    noecho();
    start_color();
    use_default_colors();
    curs_set(0);

    // Setup a palette of 256 shades of grey
    // TODO check COLORS >= 256 && if( can_change_color() )
    int mapped;
    for (int i = 0; i < 256; i++) {
        mapped = mapVal(i, 0, 255, 0, 1000);
        init_color(i, mapped, mapped, mapped);
        init_pair(i, COLOR_BLACK, i);
    }
    
    /*
     * Main webcam capture loop
     */
    for(;;)
    {
        // capture frame from camera or bail
        capture >> frame;

        if (frame.empty()) {
            std::cout << "Failed to capture an image" << std::endl;
            return -1;
        }

        // Square center crop, chop width to match height, equals bits from both sides
        cropped = frame(squareInCenter);

        // Convert the region of interest into greyscale
        cvtColor(cropped, grey, CV_BGR2GRAY);

        // Resize greyscale image into peggy-sized thumbnail, 8-bit unsigned matrix of grey level
        thumb = Mat(peggySize.width, peggySize.height, CV_8U);
        resize(grey, thumb, peggySize, 0, 0, INTER_LANCZOS4);

        //std::cout << "Thumb = " << std::endl << thumb << std::endl << std::endl; // debug
        printThumbnail(thumb);
        refresh();


    }//capture loop

    curs_set(1);
    endwin();

    return 0;
}

/* Prints the thumbnail to the terminal
 */
void printThumbnail(Mat thumb) {
    // 16 characters arranged (roughly) in order of increasing density (brightness)
    //const char palette[] = " .'^\":;!i?ZM&#%@";
    //const string palette16 (" .'^\":;!i?ZM&#%@");

    // These ramps suggested by Paul Bourke [3]
    //const char palette[] = " .:-=+*#%@";
    //const char palette[] = " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
    //int paletteSize = strlen(palette);

    uint8_t rawGrey = 0, // 0-15
            ansiGrey = 0,
            rows = thumb.rows,
            cols = thumb.cols;

    // options
    bool mirror = true,
         utf8 = true,
         hicolor = true;

    for(int i = 0; i < rows; i++ ) {
        for(int j = 0; j < cols; j++ ) {

            rawGrey = mirror ? thumb.at<uchar>(i, (cols - 1) - j) : thumb.at<uchar>(i, j);

            if (hicolor) {
                // Print two space characters with ansi grey background color
                // scaled across the 26 grey levels in the default ANSI 256 palette
                // extended ANSI colors are 0-255. 231 = #FFFFFF, 232 = #080808, 255 = #EEEEEE
                // To approximate peggy display:  grey82ansi(rawGrey, 16)
                //ansiGrey = grey82ansi(rawGrey);

                // index in brightness ramp char array
                //int idx = mapByte(rawGrey, 0, paletteSize - 1);
                //char ch = palette[idx];
                //mvprintw(i, j*2, "%c%c", ch, ch);
                attron(COLOR_PAIR(rawGrey));
                mvprintw(i, j*2, "  ");
            }
        }
    }
}

// Map a value from one range to another.  Works with negative numbers too.
// assert(mapVal(500, 0, 1000, 50, 100) == 75)
// assert(mapVal(2, 0, 10, -100, -200) == -120)
// assert(mapVal(10, 0, 0, 0, 0) == 0)
int mapVal(int value, int fromLow, int fromHigh, int toLow, int toHigh) {
    return (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
}
// Map a uint8_t (0-255) to a new range
int mapByte(uint8_t value, int low, int high) {
    return mapVal(value, 0, 255, low, high);
}

// Converts an 8-bit greyscale value to one of the 26 ANSI grey levels.
uint8_t grey82ansi(uint8_t grey8) {
    return grey82ansi(grey8, 26);
}

// Returns the best ANSI color code for the given 8-bit grey value, within the
// size of the palette.  26 ANSI grey levels are available, including black and white.
// color index 16 = black, 231 = white, 232 - 255 dark (#080808) to light (#EEEEEE) ramp 
//
// Warning: the 256 colors in the palette can be redefined.
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

    //int greyIdx = (scaled * (MAX_GREYS - 1)) / (paletteSize - 1);
    int greyIdx = mapVal(scaled, 0, paletteSize - 1, 0, MAX_GREYS - 1);
    return ANSIGREYS[greyIdx]; 
}
