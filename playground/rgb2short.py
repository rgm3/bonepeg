#!/usr/bin/env python

# Default color levels for the color cube
cubelevels = [0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff]
# Generate a list of midpoints of the above list
snaps = [(x+y)/2 for x, y in zip(cubelevels, [0]+cubelevels)[1:]]

print(snaps)

def rgb2short(r, g, b):
    """ Converts RGB values to the nearest equivalent xterm-256 color.
    """
    # Using list of snap points, convert RGB value to cube indexes
    r, g, b = map(lambda x: len(tuple(s for s in snaps if s<x)), (r, g, b))
    print(r)
    print(g)
    print(b)
    # Simple colorcube transform
    return r*36 + g*6 + b + 16

print(rgb2short(0xff, 0x00, 0xaf));
