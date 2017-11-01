#!/usr/bin/python

import struct
import sys
from PIL import Image


if len(sys.argv) < 4:
    print "Usage: ./make_silhouette.py img_file (leds_nbr|0) (frames_nbr|0) [loop bit]"
    print "Width gets resized to leds_nbr, and height to frames_nbr."
    print "Note that leds_nbr may be less than the whole strip length."
    exit()


img_name = sys.argv[1]
width = int(sys.argv[2])
height = int(sys.argv[3])

if (width == 0) and (height == 0):
    width = 144

loop = False
if len(sys.argv) > 4:
  loop = (sys.argv[4] == '1')


img = Image.open(img_name)

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

res += struct.pack('>H', width)
res += struct.pack('>H', height)

pixels = list(img_new.getdata())
if not isinstance(pixels[0], tuple):
    img_new = img_new.convert("RGB")
    pixels = list(img_new.getdata())

for (r, g, b) in pixels:
    res += chr(r) + chr(g) + chr(b)


idx1 = img_name.rfind('/')
idx2 = img_name.rfind('.')
loop_name = '_loop' if loop else ''
res_name = "/home/mtu/silhouettes/fabrique/collection/%s%s_%dx%d.SIL" % (img_name[idx1+1:idx2], loop_name, width, height)

with open(res_name, 'wb') as f:
    f.write(res)

