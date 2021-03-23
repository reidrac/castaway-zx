#!/usr/bin/env python
"""
png2sprite.py
Copyright (C) 2014 by Juan J. Martinez - usebox.net

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

"""
__version__ = "1.0"

from argparse import ArgumentParser
from PIL import Image

INK = (205, 205, 205)
PAPER = (205, 0, 0)
MASK = (0, 0, 0)

COLORS = [INK, PAPER, MASK,]

def main():

    parser = ArgumentParser(description="png2sprite",
                            epilog="Copyright (C) 2014 Juan J Martinez <jjm@usebox.net>",
                            )

    parser.add_argument("--version", action="version", version="%(prog)s "  + __version__)
    parser.add_argument("-i", "--id", dest="id", default="sprite", type=str,
                        help="variable name (default: sprite)")
    parser.add_argument("--no-mask", dest="nomask", action="store_true",
                        help="don't include mask information in the output (non masked, non rotated sprite)")

    parser.add_argument("image", help="image to convert", nargs="?")

    args = parser.parse_args()

    if not args.image:
        parser.error("required parameter: image")

    try:
        image = Image.open(args.image)
    except IOError:
        parser.error("failed to open the image")

    (w, h) = image.size

    if w % 8 or h % 8:
        parser.error("%r size is not multiple of 8" % args.image)

    if not isinstance(image.getpixel((0, 0)), tuple):
        parse.error("only RGB(A) images are supported")

    # so we support both RGB and RGBA images
    data = list(zip(list(image.getdata(0)), list(image.getdata(1)), list(image.getdata(2))))

    for c in data:
        if c not in COLORS:
            parser.error("invalid color %r in image" % (c,))

    tiles = []
    # this is wrong, but assuming nxn sprites works
    for j in range(h / 8):
        if not args.nomask:
            tiles.extend([255, 0, 255, 0, 255, 0, 255, 0])

    for x in range(0, w, 8):
        byte = []
        for j in range(h):
            sprite = 0
            mask = 0
            for i in range(8):
                d = data[x + i + j * w]
                if d == INK:
                    sprite |= 1 << (7 - i)
                elif d != MASK:
                    mask |= 1 << (7 - i)

            if not args.nomask:
                byte.append(mask)
            byte.append(sprite)

        tiles.extend(byte)
        if not args.nomask:
            tiles.extend([255, 0, 255, 0, 255, 0, 255, 0])
            if x + 8 < w:
                tiles.extend([255, 0, 255, 0, 255, 0, 255, 0])
        else:
            tiles.extend([0, 0, 0, 0, 0, 0, 0, 0])

    if not args.nomask:
        tiles.extend([255, 0, 255, 0, 255, 0, 255, 0])

    out = ""
    for part in range(0, len(tiles), 8):
        if out:
            out += ",\n"
        out += ', '.join(["0x%02x" % b for b in tiles[part: part + 8]])

    # header
    print("""
/* png2sprite.py
 *
 * %s (%sx%s) - %d bytes
 */
""" % (args.image, w, h, len(tiles)))

    #print("""const uchar %s_pad[] = { 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0 };\n""" % args.id)
    print("""const uchar %s[] = {\n%s\n};""" % (args.id, out,))

if __name__ == "__main__":
    main()

