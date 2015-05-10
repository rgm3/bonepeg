/* bonepeg.cpp
 *
 * Having too much fun learning C and playing with ascii cam to implement peggy output yet.
 * Considering renaming to "tcam."
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
#include <stdbool.h>
#include <locale.h>
#include <time.h>
#include <ncurses.h>

// Capture size from camera
#define CAM_WIDTH 800
#define CAM_HEIGHT 600

#undef max
#undef min

using namespace std;
using namespace cv;

// Declared functions
void fillColorLookupTable();
void printImage(Mat);
void restoreTerminal();
void readKeys();
void screenshot();
void setupSignalHandlers();
uint8_t luma(uint8_t, uint8_t, uint8_t);
uint8_t grey2ansi(uint8_t grey8);
uint8_t grey2ansi(uint8_t grey8, uint8_t paletteSize);
uint8_t rgb2ansi(cv::Vec3b*);

// Globals
uint8_t INCS[6] = { 0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff };
uint8_t CLUT[6][6][6]; // color lookup table
uint8_t GREYS[24];
bool g_mirror = true;
bool g_color = true;

volatile sig_atomic_t g_stop = 0;
volatile sig_atomic_t g_termResized = 0;

void sigint_handler(int sig) {
    g_stop = 1;
}

void sigwinch_handler(int sig)
{
    g_termResized = 1;
}

void restoreTerminal() {
    nocbreak();
    echo();
    endwin();
}

int main(int argc, char** argv)
{

    setupSignalHandlers();

    // set locale
    if (!setlocale(LC_CTYPE, "")) {
        // cerr << "Can't set the specified locale! Check LANG, LC_CTYPE, LC_ALL." << endl; // any practical difference?
        fprintf(stderr, "Can't set the specified locale! "
                "Check LANG, LC_CTYPE, LC_ALL.\n");
        return 1;
    }

    fillColorLookupTable();

    // prints "VIDIOC_QUERYMENU: Invalid argument" on stderr a few times with this OpenCV, but works fine
    // Seems to be an issue with OpenCV querying device capabilities
    VideoCapture capture(0);
    capture.set(CV_CAP_PROP_FRAME_WIDTH, CAM_WIDTH);
    capture.set(CV_CAP_PROP_FRAME_HEIGHT, CAM_HEIGHT);

    if (!capture.isOpened()) {
        cout << "Failed to connect to the camera." << endl;
        return -1;
    }

    
    initscr();
    start_color(); // init 8 basic colors, and COLORS / COLOR_PAIRS vars
    use_default_colors();

    // 88 colors would probably look pretty good in color
    // 8 is probably enough for greyscale
    if ( COLORS < 256 ) {
      restoreTerminal();
      printf("Terminal only supports %d colors.  Maybe \"export TERM=xterm-256color.\"\n", COLORS);
      return -1;
    }

    int trows, tcols;
    getmaxyx(stdscr,trows,tcols);
    Size termSize  = Size(tcols, trows);
    Size peggySize = Size(25, 25);
    
    // 13pt Monaco        = 8x18 = 4:9  = .444
    // 12pt Monaco        = 7x17 = 7:17 = .412
    // 11pt Monaco        = 7x15 = 7:15 = .467
    // 10pt Monaco        = 6x14 = 3:7  = .429
    // 12pt Menlo regular = 7x14 = 1:2  = .500

    float char_aspect = 4.0f / 9; 
    int effective_trows = (1.0f / char_aspect) * trows;
    float term_aspect = (float) tcols / effective_trows;
    float cam_aspect = (float) CAM_WIDTH / CAM_HEIGHT;

    // Crop a rectangle matching the effective terminal aspect ratio out of the camera capture
    int cropheight, cropwidth, x, y;
    if ( term_aspect > cam_aspect) {
      // wide: more columns than rows
      cropwidth  = CAM_WIDTH;
      cropheight = CAM_WIDTH / term_aspect;
    } else if ( term_aspect < cam_aspect ) {
      // tall: more rows than columns
      cropheight = CAM_HEIGHT;
      cropwidth  = term_aspect * cropheight;
    } else {
      // square: camera aspect > 1 so use the smaller dimension as bounds
      cropwidth  = CAM_HEIGHT; 
      cropheight = CAM_HEIGHT;
    }
    x = (CAM_WIDTH - cropwidth ) / 2;
    y = (CAM_HEIGHT - cropheight) / 2;

/*
    printw("Cropping %dx%d at (%d,%d). aspect = %f", cropwidth, cropheight, x, y, term_aspect);
    refresh();
    getch();
    //g_stop = true;
*/

    // What is this constructor and variable asignment magic
    Rect cropArea(x, y, cropwidth, cropheight);

    cbreak();              // Don't buffer until newline
    keypad(stdscr, true);  // Accept non-typewriter keys like left, right, up, down, F1
    noecho();              // Don't echo inputs to screen
    curs_set(0);           // turn off cursor
    nodelay(stdscr, true); // don't wait on key inputs

    for (int i = 0; i < COLORS; i++){
      init_pair(i, -1, i);
    }

    refresh();
    
    /*
     * Main webcam capture loop
     */

    Mat thumb = Mat(termSize.width, termSize.height, CV_8UC3);
    Mat frame, cropped;
    while (!g_stop) {
        // capture frame from camera or bail
        capture >> frame;

        if (frame.empty()) {
            cout << "Failed to capture an image" << endl;
            break;
        }

        // center crop full res area matching terminal aspect ratio
        cropped = frame(cropArea);
/*
mvprintw(1, 1, "cropped = %dx%d\n", cropped.cols, cropped.rows);
refresh();
*/

        // shrink to terminal size (squishes the vertical by character aspect ratio)
        Mat blurred = cropped.clone();
        GaussianBlur( cropped, blurred, Size(3,3), 0, 0 );
        resize(blurred, thumb, termSize, 0, 0, INTER_LINEAR);

        printImage(thumb);

        refresh();

        readKeys();

    }//capture loop

    capture.release();

    restoreTerminal();

    return 0;
}

void setupSignalHandlers() {
    // Trap SIGINT / CTRL-c
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = sigint_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    // Trap SIGWINCH / Handle window resize
    struct sigaction sa;
     memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigwinch_handler;
    sa.sa_flags = 0;
    sigaction(SIGWINCH, &sa, NULL);
}

void readKeys() {
  int ch = getch();
  if ( ch == ERR ) {
    return;
  }
  switch (ch) {
    case 'm':
      g_mirror = !g_mirror;
      break;
    case 'c':
      g_color = !g_color;
      break;
    case 'q':
    case 27: // ESC
      sigint_handler(SIGINT);
      break;
    case ' ': // space
      screenshot();
    default:
      ;
  }
}

void fillColorLookupTable() {
  int paletteIdx;
  for (int green = 0; green < 6; green++) {
    for (int red = 0; red < 6; red++) {
      for (int blue = 0; blue < 6; blue++) {
        paletteIdx = 16 + (red * 36) + (green * 6) + blue;
        CLUT[green][red][blue] = paletteIdx;
      }
    }
  }
}

void printImage(Mat thumb) {
    
    uint8_t paletteIdx = 0, // 0-15
            rows = thumb.rows,
            cols = thumb.cols;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {

            int column = g_mirror ? cols - 1 - j : j;

            if (g_color) {
              paletteIdx = rgb2ansi(&thumb.at<cv::Vec3b>(i,column));
            } else {
              //greyscale
              Vec3b pixel = thumb.at<cv::Vec3b>(i,column);
              paletteIdx = grey2ansi(luma(pixel[2], pixel[1], pixel[0]));
            }

            attron(COLOR_PAIR(paletteIdx));
            mvprintw(i, j, " ");
            attrset(A_NORMAL);
        }
    }
}

// Converts an 8-bit greyscale value to one of the 26 ANSI grey levels.
uint8_t grey2ansi(uint8_t grey8) {
    return grey2ansi(grey8, 26);
}

// Returns the best ANSI color code for the given 8-bit grey value, within the
// size of the palette.  26 ANSI grey levels are available, including black and white.
// color index 16 = black, 231 = white, 232 - 255 dark (#080808) to light (#EEEEEE) ramp 
//
// Note: the 256 colors in the palette could be redefined.
//
// @param grey8         an 8-bit grey value.  Gets scaled to 0-paletteSize.
// @param paletteSize   The number of greys in the palette.
uint8_t grey2ansi(uint8_t grey8, uint8_t paletteSize) {
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

int max3(int a, int b, int c) {
  if (a > b && a > c ) { return a; }
  if (b > a && b > c ) { return b; }
  return c;
}

int min3(int a, int b, int c) {
  if (a < b && a < c ) { return a; }
  if (b < a && b < c ) { return b; }
  return c;
}

// http://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion
// http://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
// returns saturation in range 0-255 (instead of 0 - 1.0
uint8_t getSaturation(uint8_t r, uint8_t g, uint8_t b) {
  uint8_t min, max, delta, s;

  min = min3(r, g, b);
  max = max3(r, g, b);

  if (max != 0) {
    delta = max - min;
    s = long(delta) * 255 / max;
  } else {
    s = 0;
  }

  return s;
}

// converts an RGB pixel to luminosity / grey
// ITU-R Recommendation BT. 709: Y = 0.2126 R + 0.7152 G + 0.0722 B
uint8_t luma(uint8_t r, uint8_t g, uint8_t b) {
  float y = 0.2126 * r + 0.7152 * g + 0.0722 * b;
  return (uint8_t) y;
}

//uint8_t rgb2ansi(uint8_t red, uint8_t green, uint8_t blue) {
uint8_t rgb2ansi(cv::Vec3b *pixel) {

  uint8_t small, big, s1, b1, colorComponent, idx;
  uint8_t ri, gi, bi;

  uint8_t red, green, blue;
            blue  = (*pixel)[0];
            green = (*pixel)[1];
            red   = (*pixel)[2];

  // For pixels close to grey (low saturation) use a palette index from the greyscale ramp
  uint8_t saturation = getSaturation(red, green, blue);
  if (saturation <= 24) {
    uint8_t luminosity = luma(red, green, blue);
    return grey2ansi(luminosity);
  }

  for (int c = 2; c >= 0; c--) {
//    colorComponent = (number >> (c * 8)) & 0xFF;
    colorComponent = c == 2 ? red : c == 1 ? green : blue;
    for (int i = 0; i < 5; i++) {
      small = INCS[i];
      big = INCS[i+1];
      if (colorComponent >= small && colorComponent <= big) {
        s1 = abs(small - colorComponent);
        b1 = abs(big - colorComponent);
        if (s1 < b1) {
          idx = i;
        } else {
          idx = i + 1;
        }
        if (c==2) {
          ri = idx;
        }else if (c==1) {
          gi = idx;
        } else {
          bi = idx;
        }
        break;
      }
    }
  }

  return CLUT[gi][ri][bi];
}

void screenshot() {
  char filename[128];
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  strftime(filename, sizeof(filename) - 1, "snapterm-%F %H.%M.%S.win", t);

  if ( scr_dump(filename) == ERR ) {
    //TODO handle ERR / OK / user feedback
  }

  // scr_dump output only readable by scr_restore, basically useless.
  // It'd be better to dump the control codes so you could view the "image" with "cat screenshot.txt"
  // Need to figure out how to do that.
  // Alternately, figure out how to dump a .png through OpenCV, but then I'd need to give OpenCV my terminal palette and character aspect ratio.
}
