#!/usr/bin/env python
# coding: utf-8

# Copyright © 2015 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 or 3
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>

import sys
from PIL import Image

def premultiply(image):
    pixels = image.load()
    for i in range(image.size[0]):
        for j in range(image.size[1]):
            orig = pixels[i,j]
            m = orig[3] / 255.0
            pixels[i,j] = (int(orig[0] * m) , int(orig[1] * m), int(orig[2] * m), orig[3])

def tocstring(data):
    result = ''
    line_chars = 0
    line_limit = 80

    for c in data:
        if line_chars == 0:
            result += '    "'

        s = '\\%o' % c
        result += s
        line_chars += len(s)

        if line_chars >= line_limit:
            result += '"\n'
            line_chars = 0

    if line_chars != 0:
        result += '"'

    return result

def bytes_per_pixel(image):
    if image.mode == 'RGBA':
        return 4
    elif image.mode == 'RGB':
        return 3
    else:
        raise "Unsupported image mode %s" % image.mode

def export(image, variable_name):
    image_info = (image.size[0], image.size[1], bytes_per_pixel(image))
    print("static const struct {")
    print("    unsigned int width;")
    print("    unsigned int height;")
    print("    unsigned int bytes_per_pixel; /* 3:RGB, 4:RGBA */")
    print("    unsigned char pixel_data[%d * %d * %d + 1];" % image_info)
    print("} %s = {" % variable_name)
    print("    %d, %d, %d," % image_info)
    print(tocstring(image.tobytes()))
    print("};")

def show_usage():
    print("Usage: ./png2header.py PNGFILE VARNAME > HEADER_FILE", file=sys.stderr)
    print("Convert a PNG image to an embeddable C/C++ header file", file=sys.stderr)

if len(sys.argv) < 3:
    show_usage()
    sys.exit(1)

image_filename = sys.argv[1]
variable_name = sys.argv[2]

image = Image.open(image_filename)
if image.mode == 'RGBA':
    premultiply(image)
export(image, variable_name)
