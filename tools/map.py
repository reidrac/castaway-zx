#!/usr/bin/env python3

__version__ = "1.0"

import sys
from argparse import ArgumentParser
import json
import subprocess
from collections import defaultdict
from PIL import Image

DEF_ROOM_WIDTH = 16
DEF_ROOM_HEIGHT = 10

COLORS = ( (0, 0, 0),
           (0, 0, 205), (0, 0, 255),
           (205, 0, 0), (255, 0, 0),
           (205, 0, 205), (255, 0, 255),
           (0, 205, 0), (0, 255, 0),
           (0, 205, 205), (0, 255, 255),
           (205, 205, 0), (255, 255, 0),
           (205, 205, 205), (255, 255, 255),
           )

COLOR_NAMES = ( "black", "blue", "bright-blue",
                "red", "bright-red", "magenta", "bright-magenta",
                "green", "bright-green", "cyan", "bright-cyan",
                "yellow", "bright-yellow", "white", "bright-white",)


ATTR_I = ( 0x00, 0x01, 0x01 | 0x40, 0x02, 0x02 | 0x40,
           0x03, 0x03 | 0x40, 0x04, 0x04 | 0x40, 0x05, 0x05 | 0x40,
           0x06, 0x06 | 0x40, 0x07, 0x07 | 0x40,)

ATTR_P = ( 0x00, 0x08, 0x08 | 0x40, 0x10, 0x10 | 0x40,
           0x18, 0x18 | 0x40, 0x20, 0x20 | 0x40, 0x28, 0x28 | 0x40,
           0x30, 0x30 | 0x40, 0x38, 0x38 | 0x40,)

C2I = dict(zip(COLORS, ATTR_I))
C2P = dict(zip(COLORS, ATTR_P))

BASE = 128

def find_name(data, name):
    for item in data:
        if item.get("name").lower() == name.lower():
            return item
    sys.exit("%r not found in json file" % name)

def process_tiles(data, bg_color, fg_color, base):

    def_tileset = find_name(data["tilesets"], "default")

    try:
        image = Image.open(def_tileset["image"])
    except IOError:
        sys.exit("failed to open the image")

    (w, h) = image.size

    if w % 8 or h % 8:
        sys.exit("%r size is not multiple of 8" % def_tileset["image"])

    if not isinstance(image.getpixel((0, 0)), tuple):
        sys.exit("only RGB(A) images are supported")

    # so we support both RGB and RGBA images
    data = list(zip(list(image.getdata(0)), list(image.getdata(1)), list(image.getdata(2))))

    for c in data:
        if c not in COLORS:
            sys.exit("invalid color %r in image" % (c,))

    out = []
    tiles = {}
    matrix = {}
    for y in range(0, h, 8):
        for x in range(0, w, 8):
            byte = []
            attr = []
            for j in range(8):
                row = 0
                for i in range(8):
                    if not attr:
                        attr.append(data[x + i + (j + y) * w])
                    if data[x + i + (j + y) * w] != attr[0]:
                        row |= 1 << (7 - i)
                    if data[x + i + (j + y) * w] not in attr:
                        attr.append(data[x + i + (j + y) * w])
                byte.append(row)

            if len(attr) > 2:
                sys.exit("more than 2 colors in an attribute block in (%d, %d)" % (x, y))
            elif len(attr) == 1:
                attr.append(fg_color)

            if bg_color and attr[0] != bg_color and attr[1] == bg_color:
                attr[0], attr[1] = attr[1], attr[0]
                byte = [~b & 0xff for b in byte]

            if fg_color and attr[1] != fg_color and attr[0] == fg_color:
                attr[0], attr[1] = attr[1], attr[0]
                byte = [~b & 0xff for b in byte]

            byte_i = tuple(byte)
            if byte_i not in tiles:
                tiles[byte_i] = len(tiles)
                out.extend(byte)

            matrix[(x // 8, y // 8)] = (attr, tiles[byte_i])


    tile_mapping = []
    for y in range(0, h // 8, 2):
        for x in range(0, w // 8, 2):
            tile_mapping.append(matrix[(x ,y)])
            tile_mapping.append(matrix[(x + 1 ,y)])
            tile_mapping.append(matrix[(x, y + 1)])
            tile_mapping.append(matrix[(x + 1, y + 1)])

    # convert into an list of attr, index, attr, index, ...
    tile_mapping_out = []
    for attr, index in tile_mapping:
        tile_mapping_out.append(C2P[attr[0]] | C2I[attr[1]])
        tile_mapping_out.append(index + base)

    return out, tile_mapping_out

def process_map(rw, rh, data):
    mh = data.get("height", 0)
    mw = data.get("width", 0)

    if mh < rh or mh % rh:
        sys.exit("Map size in not multiple of the room size")
    if mw < rw or mw % rw:
        sys.exit("Map size in not multiple of the room size")

    tilewidth = data["tilewidth"]
    tileheight = data["tileheight"]

    tile_layer = find_name(data["layers"], "Map")["data"]

    def_tileset = find_name(data["tilesets"], "default")
    tileprops = def_tileset.get("tileproperties", {})
    firstgid = def_tileset.get("firstgid")

    out = []
    for y in range(0, mh, rh):
        for x in range(0, mw, rw):
            current = []
            for j in range(rh):
                for i in range(rw // 2):
                    a = (tile_layer[x + (i * 2) + (y + j) * mw] - firstgid) & 0b1111
                    b = (tile_layer[x + (i * 2) + 1 + (y + j) * mw] - firstgid) & 0b1111
                    current.append((a << 4) | b)
            out.append(current)

    return out

def process_objects(rw, rh, data, sprite_limit):
    objects = find_name(data["layers"], "Objects")["objects"]

    doors = defaultdict(int)
    sprite_count = defaultdict(int)
    out = defaultdict(list)
    for ob in objects:
        if ob["type"].lower() == "platform":
            map_id = ob["x"] // data["tilewidth"] // rw \
                    + (ob["y"] // data["tileheight"] // rh) * (data["width"] // rw)
            ob_x = (ob["x"] % (data["tilewidth"] * rw)) // 16
            ob_y = (ob["y"] % (data["tileheight"] * rh)) // 16
            if ob["height"] > ob["width"]:
                ob_type = 0
                ob_param = ob["height"] // 16
                if ob["properties"].get("direction", "down") == "up":
                    ob_type = 2
                    ob_y += ((ob["height"] - 16) % (data["tileheight"] * rh)) // 16
            else:
                ob_type = 1
                ob_param = ob["width"] // 16
                if ob["properties"].get("direction", "right") == "left":
                    ob_type = 3
                    ob_x += ((ob["width"] - 16) % (data["tilewidth"] * rw)) // 16
            ob_param -= 1
            sprite_count[map_id] += 1
        elif ob["type"].lower() == "object":
            map_id = ob["x"] // data["tilewidth"] // rw \
                    + (ob["y"] // data["tileheight"] // rh) * (data["width"] // rw)
            ob_type = 4
            ob_param = ob["properties"]["id"]
            ob_x = (ob["x"] % (data["tilewidth"] * rw)) // 16
            ob_y = (ob["y"] % (data["tileheight"] * rh)) // 16

            sprite_count[map_id] += 1
        elif ob["type"].lower() == "zwall-left":
            map_id = ob["x"] // data["tilewidth"] // rw \
                    + (ob["y"] // data["tileheight"] // rh) * (data["width"] // rw)
            ob_type = 5
            ob_param = ob["properties"].get("delay", 32)
            ob_x = (ob["x"] % (data["tilewidth"] * rw)) // 16
            ob_y = (ob["y"] % (data["tileheight"] * rh)) // 16

            sprite_count[map_id] += 2
        elif ob["type"].lower() == "zwall-right":
            map_id = ob["x"] // data["tilewidth"] // rw \
                    + (ob["y"] // data["tileheight"] // rh) * (data["width"] // rw)
            ob_type = 6
            ob_param = ob["properties"].get("delay", 32)
            ob_x = (ob["x"] % (data["tilewidth"] * rw)) // 16
            ob_y = (ob["y"] % (data["tileheight"] * rh)) // 16

            sprite_count[map_id] += 2
        elif ob["type"].lower() == "enemy":
            map_id = ob["x"] // data["tilewidth"] // rw \
                    + (ob["y"] // data["tileheight"] // rh) * (data["width"] // rw)
            ob_type = 10
            ob_param = (int(ob["properties"].get("type", 0)) & 0x0f) \
                    | (int(ob["properties"].get("dir", 0)) << 4) & 0xf0
            ob_x = (ob["x"] % (data["tilewidth"] * rw)) // 16
            ob_y = (ob["y"] % (data["tileheight"] * rh)) // 16

            sprite_count[map_id] += 1
        elif ob["type"].lower() == "door":
            map_id = ob["x"] // data["tilewidth"] // rw \
                    + (ob["y"] // data["tileheight"] // rh) * (data["width"] // rw)
            ob_x = (ob["x"] % (data["tilewidth"] * rw)) // 16
            ob_y = (ob["y"] % (data["tileheight"] * rh)) // 16
            if ob["height"] > ob["width"]:
                ob_type = 11
                ob_param = ob["height"] // 16
            else:
                ob_type = 12
                ob_param = ob["width"] // 16
            ob_param -= 1

            doors[map_id] += 1
            if doors[map_id] > 1:
                sys.exit("More than one door in room %d" % map_id)
        else:
            sys.exit("Unsupported type %s" % ob["type"])

        # doors first
        if ob_type in (11, 12):
            out[map_id] = [ob_type, ob_param, ob_x, ob_y] + out[map_id]
        else:
            out[map_id].extend([ob_type, ob_param, ob_x, ob_y])

        if sprite_limit and sprite_count[map_id] > sprite_limit:
            sys.exit("Sprite count in room %d (%d > %d)" % (map_id, sprite_count[map_id], sprite_limit))

    for map_id in out.keys():
        out[map_id].append(255) # end

    return out


def main():

    parser = ArgumentParser(description="Map importer for Castaway",
                            epilog="Copyright (C) 2015 Juan J Martinez <jjm@usebox.net>",
                            )

    parser.add_argument("--version", action="version", version="%(prog)s "  + __version__)
    parser.add_argument("--room-width", dest="rw", default=DEF_ROOM_WIDTH, type=int,
                        help="room width (default: %s)" % DEF_ROOM_WIDTH)
    parser.add_argument("--room-height", dest="rh", default=DEF_ROOM_HEIGHT, type=int,
                        help="room height (default: %s)" % DEF_ROOM_HEIGHT)
    parser.add_argument("--ucl", dest="ucl", action="store_true",
                        help="UCL compress (requires ucl binary in the path)")
    parser.add_argument("-b", "--base", dest="base", default=BASE, type=int,
                        help="base character for the tiles (default: %d)" % BASE)
    parser.add_argument("--preferred-bg", dest="bg_color", type=str, default="black",
                        help="preferred background color (eg, black)")
    parser.add_argument("--preferred-fg", dest="fg_color", type=str, default="white",
                        help="preferred fireground color (eg, white)")
    parser.add_argument("--sprite-limit", dest="sprite_limit", default=0, type=int,
                        help="sprite limit per room (default: none)")
    parser.add_argument("--list-colors", dest="list_colors", action="store_true",
                        help="list color names (for --preferred-bg option)")
    parser.add_argument("map_json", help="Map to import")
    parser.add_argument("id", help="variable name")

    args = parser.parse_args()

    if args.list_colors:
        print("Color list: %s" % ', '.join(COLOR_NAMES))
        return

    bg_color = None
    if args.bg_color:
        if args.bg_color.lower() not in COLOR_NAMES:
            parser.error("invalid color name %r" % args.bg_color)
        else:
            bg_color = COLORS[COLOR_NAMES.index(args.bg_color.lower())]

    fg_color = None
    if args.fg_color:
        if args.fg_color.lower() not in COLOR_NAMES:
            parser.error("invalid color name %r" % args.fg_color)
        else:
            fg_color = COLORS[COLOR_NAMES.index(args.fg_color.lower())]

    with open(args.map_json, "rt") as fd:
        data = json.load(fd)

    out = process_map(args.rw, args.rh, data)

    if args.ucl:
        compressed = []
        for block in out:
            p = subprocess.Popen(["ucl",], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
            output, err = p.communicate(bytearray(block))
            compressed.append([ord(chr(b)) for b in output])
        out = compressed

    print("/* imported from %s */" % args.map_json)
    print("#define ROOM_WIDTH %d" % args.rw)
    print("#define ROOM_HEIGHT %d" % args.rh)
    print("#define ROOM_SIZE %d" % (args.rw * args.rh // 2))
    print("#define MAP_WIDTH %d" % (data["width"] // args.rw))

    data_out = ""
    for i, block in enumerate(out):
        data_out_part = ""
        for part in range(0, len(block), args.rw // 2):
            if data_out_part:
                data_out_part += ",\n"
            data_out_part += ', '.join(["0x%02x" % b for b in block[part: part + args.rw // 2]])
        data_out += "const unsigned char %s_%d[%d] = {\n" % (args.id, i, len(block))
        data_out += data_out_part + "\n};\n"

    data_out += "const unsigned char *%s[%d] = { " % (args.id, len(out))
    data_out += ', '.join(["%s_%d" % (args.id, i) for i in range(len(out))])
    data_out += " };\n"
    print(data_out)

    total_maps = len(out)

    out, out_tile_mapping = process_tiles(data, bg_color, fg_color, args.base)

    print("/* map tiles from %s */" % find_name(data["tilesets"], "default")["image"])
    print("#define %s_TILE_BASE %d" % (args.id.upper(), args.base))
    print("#define %s_TILE_LEN %d" % (args.id.upper(), len(out) // 8))
    print("const unsigned char %s_tiles[%d] = {" % (args.id, len(out)))
    data_out = ""
    for part in range(0, len(out), 8):
        if data_out:
            data_out += ",\n"
        data_out += ', '.join(["0x%02x" % b for b in out[part: part + 8]])
    print("%s\n};\n" % data_out)

    print("const unsigned char %s_tile_mapping[%d] = {" % (args.id, len(out_tile_mapping)))
    data_out = ""
    for part in range(0, len(out_tile_mapping), 8):
        if data_out:
            data_out += ",\n"
        data_out += ', '.join(["0x%02x" % b for b in out_tile_mapping[part: part + 8]])
    print("%s\n};\n" % data_out)

    out = process_objects(args.rw, args.rh, data, args.sprite_limit)

    for map_id, block in out.items():
        print("const uchar %s_objects%d[%d] = { %s };" % (args.id,
            map_id, len(block), ", ".join(map(str, block))))

    ob_map_list = []
    for m in range(total_maps):
        if m in out:
            ob_map_list.append("%s_objects%d" % (args.id, m))
        else:
            ob_map_list.append("0")
    print("const uchar *%s_objects[%d] = { %s };" % (args.id,
        total_maps, ", ".join(ob_map_list)))

    print("#define TOTAL_ROOMS %d\n" % total_maps)

if __name__ == "__main__":
    main()

