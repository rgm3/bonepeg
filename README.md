bonepeg
=======

A program to stream video from a webcam to the [Peggy 2][1] via [i2c][2] on the [BeagleBone Black][3].
Currently it's more of an ASCII-cam, since i2c output isn't implemented, but [terminal preview of video][5] feed is.

A re-implementation of the method used in [Video Peggy in action][4] without the laptop and Arduino.
   
   [1]: http://wiki.evilmadscientist.com/Peggy_2 "Peggy 2 from Evil Mad Scientist"
   [2]: http://en.wikipedia.org/wiki/I%C2%B2C
   [3]: http://beagleboard.org/Products/BeagleBone%20Black
   [4]: http://www.evilmadscientist.com/2009/video-peggy-in-action/
   [5]: http://asciinema.org/a/3904

Building
========
Requires the OpenCV libraries.

## On Mac OS X with macports:
```
    # sudo port install opencv
    # make
```

## Linux Mint
```
    # sudo apt-get install g++ libncurses5-dev libopencv-dev
    # make
```

Links
=====
*  http://www.evilmadscientist.com/source/Peggy2_RecvTWI.pde
*  http://www.planetclegg.com/projects/Twi2Peggy.html
*  http://docs.opencv.org/_downloads/how_to_scan_images.cpp 

v4l2-ctl - Listing devices, capabilities, and settings
======================================================

To display all available v4l2 information:
   # v4l2-ctl --all

Formats that the device can provide:
   # v4l2-ctl --list-formats

Terminal chromophilia
=====================
Ask your doctor if these are right for you:
*  http://xyne.archlinux.ca/projects/tiv/
*  https://github.com/busyloop/lolcat/
*  https://asciinema.org/
*  https://www.brow.sh/
