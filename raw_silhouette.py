#!/usr/bin/python

import struct
import sys
from PIL import Image


if len(sys.argv) < 4:
    print "Usage: ./resize.py img_file (leds_nbr|0) (frames_nbr|0) [rotate bit [loop bit]]"
    print "Except with rotate 1, width gets resized to leds_nbr, and height to frames_nbr."
    exit()


img_name = sys.argv[1]
width = int(sys.argv[2])
height = int(sys.argv[3])

if (width == 0) and (height == 0):
    width = 144

rotate = False
if len(sys.argv) > 4:
  rotate = (sys.argv[4] == '1')

loop = False
if len(sys.argv) > 5:
  loop = (sys.argv[5] == '1')


img = Image.open(img_name)

if rotate:
    img.rotate(90)


if (height == 0):
    wprct = width / float(img.size[0])
    height = int(img.size[1] * wprct)

if (width == 0):
    hprct = height / float(img.size[1])
    width = int(img.size[0] * hprct)

img_new = img.resize((width, height), Image.BICUBIC)


res = ''

if loop:
    res += '\x80'
else:
    res += '\0'

res += struct.pack('>H', height)

pixels = list(img_new.getdata())
for (r, g, b) in pixels:
    res += chr(r) + chr(g) + chr(b)


idx = img_name.rfind('.')
rot_name = '_rot' if rotate else ''
loop_name = '_loop' if loop else ''
res_name = "%s%s%s_%dx%d.raw" % (img_name[:idx], rot_name, loop_name, width, height)

with open(res_name, 'wb') as f:
    f.write(res)

