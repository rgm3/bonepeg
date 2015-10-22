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
#include <sys/time.h>
#include <ncurses.h>

// Capture size from camera
#define CAM_WIDTH 640
#define CAM_HEIGHT 480

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
uint8_t getSaturation(uint8_t, uint8_t, uint8_t);
uint8_t grey2ansi(uint8_t grey8);
uint8_t grey2ansi(uint8_t grey8, uint8_t paletteSize);
uint8_t rgb2ansi(cv::Vec3b);
uint8_t rgb_to_idx(cv::Vec3b);

// Globals
const uint8_t MAX_GREYS = 26, MIN_GREYS = 2;
const uint32_t MICROS_PER_SEC = 1000000;

uint8_t INCS[6] = { 0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff }; // component RGB color stops in cube
uint8_t MIDS[5]; // Holds midpoints between color stops, for finding closest color
uint8_t CLUT[6][6][6]; // color lookup table


struct rgb { uint8_t r; uint8_t g; uint8_t b; };

bool g_mirror = true;
bool g_color = true;
volatile bool g_fps = false;
uint8_t g_saturation_threshold = 64; // Trade color for details when low saturation

volatile sig_atomic_t g_stop = 0;
volatile sig_atomic_t g_termResized = 0;

void sigint_handler(int sig) {
    g_stop = 1;
}

void sigwinch_handler(int sig) {
    g_termResized = 1;
}

void restoreTerminal() {
    nocbreak();
    echo();
    endwin();
}

/**
 * Finds the index of the closet color in the 6x6x6 color cube, or a close grey value.
 */
uint8_t rgb_to_idx(cv::Vec3b c) {
   uint8_t r = 5, g = 5, b = 5;

  uint8_t saturation = getSaturation(c[2], c[1], c[0]);
  if (saturation <= g_saturation_threshold) {
    uint8_t luminosity = luma(c[2], c[1], c[0]);
    return grey2ansi(luminosity);
  }

  // 6x6x6 color cube coordinates, 0-5 on each axis
  for (int i = 5; i >= 0; i--) {
    if (c[2] < MIDS[i]) {
      r = i;
    }
    if (c[1] < MIDS[i]) {
      g = i;
    }
    if (c[0] < MIDS[i]) {
      b = i;
    }
  }

  return r * 36 + g * 6 + b + 16;
}

/**
 * @return Number microseconds between two timespecs
 */
int64_t timevalDiff(struct timeval *start, struct timeval *end) {
  return ((end->tv_sec * MICROS_PER_SEC) + end->tv_usec) -
           ((start->tv_sec * MICROS_PER_SEC) + start->tv_usec);
}

int main(int argc, char** argv) {

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
    Size capRes = Size(capture.get(CV_CAP_PROP_FRAME_WIDTH), capture.get(CV_CAP_PROP_FRAME_HEIGHT));

    if (!(capRes.width == CAM_WIDTH && capRes.height == CAM_HEIGHT)) {
        fprintf(stderr, "Capture resolution %dx%d not available.\nCapture opened at %dx%d.\n", CAM_WIDTH, CAM_HEIGHT, capRes.width, capRes.height);
        return -1;
    }

    if (!capture.isOpened()) {
        cerr << "Failed to connect to the camera." << endl;
        return -1;
    } else {
        cout << "Video source is " << capture.get(CV_CAP_PROP_FRAME_WIDTH) <<
          "x" << capture.get(CV_CAP_PROP_FRAME_HEIGHT) << endl;
    }

    initscr();
    start_color(); // init 8 basic colors, and COLORS / COLOR_PAIRS vars
    use_default_colors();

    // 88 colors would probably look pretty good in color
    // 8 is probably enough for greyscale
    if ( COLORS < 256 ) {
      restoreTerminal();
      printf("Terminal only supports %d colors.  Try: export TERM=xterm-256color\n", COLORS);
      return -1;
    }

    int trows, tcols;
    getmaxyx(stdscr,trows,tcols);
    
    //float char_aspect = 3.0f / 7;  // 15pt Monaco = 18x42 = 3:7  = .42857
    //float char_aspect = 4.0f / 9;  // 13pt Monaco = 8x18  = 4:9  = .444
    //float char_aspect = 7.0f / 17; // 12pt Monaco = 7x17  = 7:17 = .412
    //float char_aspect = 7.0f / 15; // 11pt Monaco = 7x15  = 7:15 = .467
    //float char_aspect = 3.0f / 7;  // 10pt Monaco = 6x14  = 3:7  = .42857
    //float char_aspect = 8.0f / 17; // 14pt Menlo regular = 8x17 = .470588
    //float char_aspect = 5.0f / 11; // 9pt  Menlo regular = 5x11 = .4545
    float char_aspect = 1.0f / 2;  // 12pt Menlo regular = 7x14 = 1:2  = .500

    int effective_trows = trows / char_aspect;
    float term_aspect = char_aspect * tcols / (float) trows;
    float cam_aspect = (float) CAM_WIDTH / CAM_HEIGHT;
    float pixel_aspect = 1.0; // OpenCV can return 3:4 (0.75) pixels when asking for 800x600 on MBP. MBP 'FaceTime HD' cam supports 320x240, 640x480, and 1280x720.

    // Crop a rectangle matching the effective terminal aspect ratio out of the camera capture
    int cropheight, cropwidth, x_offset, y_offset;
    if ( term_aspect > cam_aspect ) {
      // wide: more columns than rows
      cropwidth = CAM_WIDTH;
      cropheight = ((float)cropwidth * trows) / (pixel_aspect * tcols * char_aspect);
    } else if ( term_aspect < cam_aspect ) {
      // tall: more rows than columns
      cropheight = CAM_HEIGHT;
      cropwidth  = (pixel_aspect * cropheight * tcols * char_aspect) / trows;
    } else {
      // square: camera aspect > 1 so use the smaller dimension as bounds
      cropwidth  = CAM_HEIGHT; 
      cropheight = CAM_HEIGHT;
    }
    // Cropping offsets
    x_offset = (CAM_WIDTH - cropwidth ) / 2;
    y_offset = (CAM_HEIGHT - cropheight) / 2;


    printw("Cropping %dx%d at offset (%d,%d) from source capture %dx%d.\nTerm aspect = %f, crop aspect = %f\n", 
        cropwidth, cropheight, x_offset, y_offset, CAM_WIDTH, CAM_HEIGHT, term_aspect, cropwidth * 1.0 / cropheight);
    printw("Terminal: %dx%d with character aspect %f, effectively %dx%d\n",
        tcols, trows, char_aspect, tcols, effective_trows);
    refresh();
    readKeys(); // pause for keypress
    //g_stop = true;

    cbreak();              // Don't buffer until newline
    keypad(stdscr, true);  // Accept non-typewriter keys like left, right, up, down, F1
    noecho();              // Don't echo inputs to screen
    curs_set(0);           // turn off cursor
    nodelay(stdscr, true); // don't wait on key inputs

    for (int i = 0; i < COLORS; i++){
      init_pair(i, -1, i);
      //init_pair(i, i, -1); // foreground
    }

    refresh();
    
    /*
     * Main webcam capture loop
     */

    struct timeval t_start, t_end;
    int64_t elapsed_usec = 0;
    uint64_t frameCount = 0;
    float avgfps = 0;
    // What is this constructor and variable asignment magic
    Rect cropArea(x_offset, y_offset, cropwidth, cropheight);
    Size termSize  = Size(tcols, trows);
    Size peggySize = Size(25, 25);
    Mat thumb = Mat(termSize.width, termSize.height, CV_8UC3);
    Mat frame, cropped;
    while (!g_stop) {
        if (g_fps && frameCount % 8 == 1) {
          gettimeofday(&t_start, NULL);
        }
        // capture frame from camera or bail
        capture >> frame;

        if (frame.empty()) {
            cout << "Failed to capture an image" << endl;
            break;
        }

        // center crop full res area matching terminal aspect ratio
        cropped = frame(cropArea);

        // shrink to terminal size (squishes the vertical by character aspect ratio)
        resize(cropped, thumb, termSize, 0, 0, INTER_LINEAR);

        // Output to screen
        printImage(thumb);

        if (g_fps) {
          attrset(COLOR_PAIR(0));
          mvprintw(0, 0, "fps: %.2f", avgfps);
          if (frameCount % 8 == 0) {
            avgfps = MICROS_PER_SEC * 8.0f / elapsed_usec;
            elapsed_usec = 0;
          }
        }

        refresh();

        readKeys();

        if (g_fps && frameCount % 8 == 0) {
          gettimeofday(&t_end, NULL);
          elapsed_usec += timevalDiff(&t_start, &t_end);
        }
        

      ++frameCount; 
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
    case 'Q':
    case 27: // ESC
      sigint_handler(SIGINT);
      break;
    case 'f':
      g_fps = !g_fps;
      break;
    case ' ': // space
      screenshot();
      break;
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

  // midpoints
  for (int i = 0; i < 5; i++) {
      MIDS[i] = (INCS[i] + INCS[i+1]) / 2;
  }
}

void printImage(Mat thumb) {
    
    uint8_t paletteIdx = 0; // 0-15
    int rows = thumb.rows,
        cols = thumb.cols;

    int lastIdx = 0;
    for (int i = 0; i < rows; i++) {
        move(i,0);
        for (int j = 0; j < cols; j++) {

            int column = g_mirror ? cols - 1 - j : j;

            Vec3b pixel = thumb.at<Vec3b>(i,column);
//            paletteIdx = grey2ansi(luma(pixel.val[2], pixel.val[1], pixel.val[0]), 26);

            if (g_color) {
            //  paletteIdx = rgb2ansi(thumb.at<cv::Vec3b>(i,column));
              paletteIdx = rgb_to_idx(thumb.at<cv::Vec3b>(i,column));
            } else {
              //greyscale
 //             Vec3b pixel = thumb.at<Vec3b>(i,column);
              paletteIdx = grey2ansi(luma(pixel.val[2], pixel.val[1], pixel.val[0]));
            }

            if (paletteIdx != lastIdx) {
              attron(COLOR_PAIR(paletteIdx));
              lastIdx = paletteIdx;
            }

            //addch(ACS_CKBOARD);
            addch(' ');
        }
        //attrset(A_NORMAL);
    }
}

// Converts an 8-bit greyscale value to one of the 26 ANSI grey levels.
uint8_t grey2ansi(uint8_t grey8) {
    return grey2ansi(grey8, MAX_GREYS);
}

/**
 * Returns the best ANSI color code for the given 8-bit grey value, scaled to a given number of levels.
 * 26 grey levels are available in the default ANSI 256 palette, including black and white.
 * color index 16 = black, 231 = white, 232 - 255 dark (#080808) to light (#EEEEEE) ramp 
 *
 * Note: the 256 colors in the palette could be redefined.
 *
 * @param grey8        an 8-bit grey value.  Gets scaled to [0..greyLevels]
 * @param greyLevels   The number of grey levels desired. Range: [2..26]
 */
uint8_t grey2ansi(uint8_t grey8, uint8_t greyLevels) {
const uint8_t ANSIGREYS[MAX_GREYS] = { 16, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 
                                       244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 231 };
    uint8_t scaled = 0;

    // bounds check
    if (greyLevels > MAX_GREYS) { greyLevels = MAX_GREYS; }
    if (greyLevels < MIN_GREYS) { greyLevels = MIN_GREYS; }

    // Convert [0..255] into [0..greyLevels] -- usually [0..26] or [0..16]
    // if the number of grey levels is a power of two [2,4,8,16] we can avoid division by shifting
    // generic power of two test: paletteSize & (paletteSize - 1)) == 0)
    // Just hard-code the four supported
    switch(greyLevels) {
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
            scaled = (int)((grey8 * greyLevels) / (255 + 1)); // 0-(paletteSize-1)
    }

    // If using max greys in palette, the scaled value is equal to the index of the ansi grey color
    if (greyLevels == MAX_GREYS) {
        return ANSIGREYS[scaled];
    }

    int greyIdx = (scaled * (MAX_GREYS - 1)) / (greyLevels - 1);
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
// returns saturation in range 0-255 (instead of 0 - 1.0)
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
uint8_t rgb2ansi(cv::Vec3b pixel) {

  uint8_t small, big, s1, b1, colorComponent, idx;
  uint8_t ri, gi, bi;

  uint8_t red, green, blue;
            blue  = pixel[0];
            green = pixel[1];
            red   = pixel[2];

  // For pixels close to grey (low saturation) use a palette index from the greyscale ramp
  uint8_t saturation = getSaturation(red, green, blue);
  if (saturation <= g_saturation_threshold) {
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
