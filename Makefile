TAP_NAME=castawy
GENERATED=menu.h play.h phead.h pbody.h stage.h numbersx2.h shipwreck.h platform.h objects.h obj_tiles.h zap.h zwall.h alien.h fly_robot.h explo.h prdr.h song1.h song2.h song3.h dr_text.h gameover.h sentinel.h sentinel2.h
CFLAGS=+zx -O3 -m -vn -Wall

export PATH:=tools:$(PATH)

# only if z88dk is not system-wide installed
#export PATH:=../z88dk/bin:$(PATH)
#export ZCCCFG:=../z88dk/lib/config/

LOAD_ADDR=24100 # REMEMBER: must be changed in other places too!

# this is the malloc limit - load addr
MAX_MEM=$(shell echo $$((51036-$(LOAD_ADDR))))

all:
	make -C tools
	make -C lib
	make $(TAP_NAME).tap
	chksize $(MAX_MEM) umain.bin

$(TAP_NAME).tap: loader.tap main.tap
	cat loader.tap main.tap > $@

main.tap: main.bin
	bin2tap -headerless -o $@ $<

basloader.tap: loader.bas
	bas2tap -q -a -s$(TAP_NAME) $< $@

loader.tap: loader.asm basloader.tap loading_screen.tap main.tap
	echo "DEFC LOADING_SIZE = $(shell stat -c "%s" loading_ucl.bin)" > loader.opt
	echo "DEFC APP_SIZE = $(shell stat -c "%s" main.bin)" >> loader.opt
	echo "DEFC LOAD_ADDR = $(LOAD_ADDR)" >> loader.opt
	z80asm -b -ns -Mbin -ilib/ucl -oloader.bin $<
	bin2tap -a 64856 -o loading.tap loader.bin
	cat basloader.tap loading.tap loading_screen.tap > $@

main.bin: main.c int.h misc.h lib/ucl.h sprites.h beeper.h numbers.h font.h $(GENERATED)
	zcc $(CFLAGS) $< -o u$@ -lsp1 -lmalloc -llib/ucl -zorg=$(LOAD_ADDR)
	ucl < umain.bin > main.bin

loading_screen.tap: loading.png
	png2scr.py loading.png
	ucl < loading.png.scr > loading_ucl.bin
	bin2tap -headerless -o $@ loading_ucl.bin

menu.h: menu.png
	png2scr.py menu.png
	ucl < menu.png.scr > menu.bin
	bin2h.py menu.bin menu > menu.h

play.h: play.png
	png2scr.py play.png
	ucl < play.png.scr > play.bin
	bin2h.py play.bin play > play.h

phead.h: phead.png
	png2sprite.py -i phead phead.png > phead.h

pbody.h: pbody.png
	png2sprite.py -i pbody pbody.png > pbody.h

stage.h: maps/stage.json tiles.png
	map.py --sprite-limit 5 --preferred-bg black --preferred-fg white --ucl maps/stage.json map > stage.h

gameover.h: gameover.png
	png2c.py --ucl -b 128 --no-print-string --preferred-bg black -i dead --array gameover.png > gameover.h

numbersx2.h: numbersx2.png
	png2c.py -b 238 --no-print-string --preferred-bg black -i numbersx2 --map numbersx2.png > numbersx2.h

shipwreck.h: shipwreck.png
	png2c.py -b 224 --no-print-string --preferred-bg blue --preferred-fg white  -i wreck --array shipwreck.png > shipwreck.h

platform.h: platform.png
	png2sprite.py -i platform platform.png > platform.h

objects.h: objects.png
	png2sprite.py --no-mask -i objects objects.png > objects.h

obj_tiles.h: obj_tiles.png
	png2c.py -b 200 --no-print-string --preferred-bg black -i obj_tiles obj_tiles.png > obj_tiles.h

zap.h: zap.png
	png2sprite.py -i zap zap.png > zap.h

zwall.h: zwall.png
	png2sprite.py --no-mask -i zwall zwall.png > zwall.h

explo.h: explo.png
	png2sprite.py -i explo explo.png > explo.h

alien.h: alien.png
	png2sprite.py -i alien alien.png > alien.h

fly_robot.h: fly_robot.png
	png2sprite.py -i fly_robot fly_robot.png > fly_robot.h

sentinel.h: sentinel.png
	png2sprite.py -i sentinel sentinel.png > sentinel.h

sentinel2.h: sentinel2.png
	png2sprite.py -i sentinel2 sentinel2.png > sentinel2.h

prdr.h: prdr.asm
	z80asm -b -ns -Mbin -oprdr.bin $<
	ucl < prdr.bin > prdr_ucl.bin
	bin2h.py prdr_ucl.bin prdr > prdr.h

song1.h: song1.asm
	z80asm -b -ns -Mbin -osong1.bin $<
	ucl < song1.bin > song1_ucl.bin
	bin2h.py song1_ucl.bin song1 > song1.h

song2.h: song2.asm
	z80asm -b -ns -Mbin -osong2.bin $<
	ucl < song2.bin > song2_ucl.bin
	bin2h.py song2_ucl.bin song2 > song2.h

song3.h: song3.asm
	z80asm -b -ns -Mbin -osong3.bin $<
	ucl < song3.bin > song3_ucl.bin
	bin2h.py song3_ucl.bin song3 > song3.h

dr_text.h: dr_text.txt
	ucl < dr_text.txt > dr_text.bin
	bin2h.py dr_text.bin dr_text > dr_text.h

clean:
	rm -f *.tap *.tzx *.scr zcc_opt.def *.map *.bin $(GENERATED)

cleanall: clean
	make -C tools clean
	make -C lib clean

