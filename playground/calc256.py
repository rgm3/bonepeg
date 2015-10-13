#!/usr/bin/env python

# https://gist.github.com/TerrorBite/e738e25881d4aecf9043

from sys import argv, stdin, stdout, stderr
from traceback import print_exc
from os import path, isatty
from textwrap import dedent
from collections import Counter
from math import sqrt
import inspect

# Luminance threshold values
# Higher values will combat higher perceived brightness
LUM_R, LUM_G, LUM_B = 0.55, 0.8, 0.3
# Cube constants
BLACK, WHITE = 16, 16+215

# Default color levels for the color cube
cubelevels = [0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff]
# Generate list of midpoints of the above list
snaps = [(x+y)/2 for x, y in zip(cubelevels, [0]+cubelevels)[1:]]

# Color LookUp Table
CLUT = [(0,0,0)]*256

################################################################################
#                              Utility Functions
################################################################################
def die(msg):
    print dedent(msg),
    raise SystemExit(1)

def islight(rgb):
    r, g, b, = rgb
    #lum = (LUM_R*r + LUM_G*g + LUM_B*b)/384.0
    lum = sqrt(sum(map(lambda x:x*x, (LUM_R*r, LUM_G*g, LUM_B*b)))/65536)
    return lum > 0.5

def islight_test(rgb):
    r, g, b = rgb
    return (LUM_R*r + LUM_G*g + LUM_B*b)/384.0

################################################################################
#                         String Conversion Functions
################################################################################
def rgb2str(rgb):
    fmt = "\033[48;2;{2};{3};{4}m\033[38;5;{1}m{0}\033[0m"
    return fmt.format(rgb2hex(rgb), BLACK if islight(rgb) else WHITE,*rgb)

def cube2str(num, width=4):
    return index2str(num+BLACK, width)

def index2str(n, width=5):
    fmt = "\033[38;5;{{1}}m\033[48;5;{{0}}m{{0:^{}}}".format(width)
    return fmt.format(n, BLACK if islight(CLUT[n]) else WHITE )

def hex2str(n):
    rgb = CLUT[n]
    fmt = "\033[38;5;{2}m\033[48;5;{0}m{1:^9}"
    return fmt.format(n, rgb2hex(rgb), BLACK if islight(rgb) else WHITE )

def index2str_ansi(n, bank, width=5):
    fmt = "\033[{};{}{}m{{:^{}}}".format(30 if n%8 else 37,
            10 if bank else 4, n, width)
    return fmt.format(n+bank)

################################################################################
#                              Terminal Handling
################################################################################
def readuntil(*end):
    "Reads up to and including the specified character."
    ch, c, data = None, 0, []
    while ch not in end:
        ch = stdin.read(1)
        data.append(ch)
        c+=1
    return ''.join(data)

def _readcolor():
    readuntil("\033")
    rgb = (0, 0, 0)
    d = stdin.read(2)
    if d != ']4':
        if d != '[0': print "Got {!r}, expecting ']4'\r".format(d)
        # Not the response we were expecting, so abort
        return (None, None)
    stdin.read(1)
    n = int(readuntil(';').strip(';'))
    data = readuntil("\033", "\007").split(':')
    if data[0] == 'rgb':
        # we know how to read this
        rgb = tuple(int(x[:2], 16) for x in data[1].split('/'))
        #print 'Queried color: {} = {}'.format(n, rgb)
        #print '{0:>4d}\r'.format(n),
    return (n, rgb)

def querycolor(n):
    stderr.write("\033]4;{};?\007".format(n))
    return _readcolor(n)[1]

def queryall():
    stderr.write(("\033]4;{};?\007"*256).format(*range(256)))
    stderr.write("\033[5n") # request Device Status Report at the end
    last=0
    while True:
        n, c = _readcolor()
        if n is None: break
        yield n, c

def read_colormap():
    print "Reading color map from terminal, please wait... ",
    import tty, termios
    fd = stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(fd)
        for n, c in queryall():
            CLUT[n] = c
        print '\r\033[K',
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)

################################################################################
#                         Color Conversion - From RGB
################################################################################
def rgb2hex(rgb):
    return "#{0:02x}{1:02x}{2:02x}".format(*rgb);

def rgb2cube(rgb):
    r, g, b = rgb
    # Snap to closest equivalent color
    r, g, b = map(lambda x: len(tuple(s for s in snaps if s<x)), (r, g, b))
    return (r*36 + g*6 + b)

################################################################################
#                                Theme Guessing
################################################################################
def guesstheme():
    ours = [rgb2cube(x)+BLACK for x in CLUT[:16]]
    #print ours
    themes = {
            "Solarized": [23, 166, 100, 136, 32, 168, 36, 145, 59, 166, 60, 66, 102, 62, 109, 231],
            "Tango": [16, 160, 64, 178, 61, 96, 30, 188, 59, 196, 113, 221, 74, 139, 80, 231],
            "XTerm": [16, 160, 40, 184, 20, 164, 44, 188, 102, 196, 46, 226, 63, 201, 51, 231],
            "CGA colors": [16, 124, 34, 130, 19, 127, 37, 145, 59, 203, 83, 227, 63, 207, 87, 231],
            "Pastel": [59, 59, 72, 180, 110, 175, 116, 188, 66, 181, 79, 223, 111, 212, 116, 231],
            }
    scores = Counter()
    for name, theme in themes.iteritems():
        scores[name] = sum([1 for a, b in zip(theme, ours) if a==b])
    #print scores.most_common()
    winner = scores.most_common()[0]
    return (winner[0], winner[1]/16.0) if winner[1] > 8 else None

################################################################################
#                          Color Conversion - To RGB
################################################################################
def hex2rgb(hexstr):
    s = hexstr.strip('#')
    return tuple(int(x, 16) for x in (s[0:2], s[2:4], s[4:6]))

def cube2rgb(num):
    """
    Returns a default value for the color cube.
    Does not take into account the actual color displayed
    by the terminal (use the CLUT for that).
    """
    return (cubelevels[x] for x in (num/36, (num/6)%6, num%6))

################################################################################
#                                Theme Guessing
################################################################################
def guesstheme():
    ours = [rgb2cube(x)+BLACK for x in CLUT[:16]]
    #print ours
    themes = {
            "Solarized": [23, 166, 100, 136, 32, 168, 36, 145, 59, 166, 60, 66, 102, 62, 109, 231],
            "Tango": [16, 160, 64, 178, 61, 96, 30, 188, 59, 196, 113, 221, 74, 139, 80, 231],
            "XTerm": [16, 160, 40, 184, 20, 164, 44, 188, 102, 196, 46, 226, 63, 201, 51, 231],
            "CGA colors": [16, 124, 34, 130, 19, 127, 37, 145, 59, 203, 83, 227, 63, 207, 87, 231],
            "Pastel": [59, 59, 72, 180, 110, 175, 116, 188, 66, 181, 79, 223, 111, 212, 116, 231],
            }
    scores = Counter()
    for name, theme in themes.iteritems():
        scores[name] = sum([1 for a, b in zip(theme, ours) if a==b])
    #print scores.most_common()
    winner = scores.most_common()[0]
    return (winner[0], winner[1]/16.0) if winner[1] > 8 else None

################################################################################
#                               Print Functions
################################################################################
def space(pad, nl=0):
    stdout.write("\033[0m" + ' '*pad + '\n'*nl)

def nl(count=1, pad=0):
    stdout.write("\033[0m" + '\n'*count + ' '*pad)

def boxnl(begin=False, end=False):
    stdout.write( '\n   '.join(
        ("\033)0   \016\033[38;5;68;48;5;145ml{0:q^116}k\033[0m".format(
            "\017\033[38;5;231;48;5;68;1m XTerm-256 Colormap \033[0;38;5;68;48;5;145m\016")
            if begin else "\033[38;5;68;48;5;145m\016x\033[0m",
        "\033[38;5;68;48;5;145mm{0:q<72}j\017\033[0m\033)B\n\n".format('')
            if end else "\033[38;5;68;48;5;145mx\017\033[0m")
    ))

def print_ansi():
    print "Colors printed using standard ANSI codes"
    for a in (0, 8):
        for x in xrange(8):
            stdout.write( index2str_ansi(x, a) )
        nl()

def print_hexcolors():
    print "\n\033[0m{0:^80}\033[0m".format("-:| Your terminal's color scheme |:-")
    space(4)
    for x in xrange(16):
        stdout.write( hex2str(x) )
        if x==7:
            nl(pad=4)
    nl()
    guess = guesstheme()
    print "\033[0m{0:^80}\033[0m".format(
            "My best guess for this theme is: {0} ({1:.0%} confidence)\n".format(*guess)
            if guess else "I don't recognise the color theme you're using.\n"
        )

def print_theme():
    print "\033[0m{0:^80}\033[0m".format("Matching your color scheme (top row) to the color cube (bottom row):")
    for x in xrange(16): stdout.write( index2str(x, 5) )
    nl()
    for x in xrange(16): stdout.write( index2str(BLACK+rgb2cube(CLUT[x]), 5) )
    nl(2)

def print_colormap():
    ### Print Xterm-256 colormap

    boxnl(begin=True)

    # Base 16 colors
    for x in xrange(16):
        stdout.write( index2str(x, 9) )
        if x==7: boxnl()
    boxnl()

    # 6x6x6 color cube
    for row in xrange(12):
        for group in (0, 72, 144):
            for col in xrange(6):
                index = row*6 + col + group
                stdout.write( cube2str(index) )
        boxnl()

    # Greyscale ramp
    for x in xrange(232, 256):
        stdout.write( index2str(x, 6) )
        if x==243: boxnl()
    boxnl(end=True)

################################################################################
#                                     Main
################################################################################
def main():
    read_colormap()

    if len(argv) > 1:
        for hexstr in argv[1:]:
            rgb = hex2rgb(argv[1])
            stdout.write( "The best match for the color \033[1m{0}\033[21m is {1}\033[0m\n"\
                    .format(rgb2hex(rgb), cube2str(rgb2cube(rgb),5)) )

    elif len(argv) == 1:
        print_hexcolors()
        print_colormap()
        print_theme()

    else: die("""\
    Usage:
      {0} <hexcolor>

            stdout.write("\033[0m\x0eq   \n   q\x0f" )
    Color values range from 0 (none) to 5 (fullbright).
    """.format(path.basename(argv[0])))

try:
    if __name__ == '__main__': main()
except Exception as e:
    print_exc()
    trace = inspect.trace()
    trace.reverse()
    for frame in trace:
        func_locals = frame[0].f_locals
        print 'Locals for {0}: {1}'.format(frame[3], repr(func_locals))
