all: ucl.lib

# only if z88dk is not system-wide installed
export PATH:=$(abspath ../../z88dk/bin):$(PATH)
export ZCCCFG:=$(abspath ../../z88dk/lib/config)/

ucl.lib: ucl.asm
	echo ucl > ucl.lst
	z80asm -d -xucl @ucl.lst
	rm ucl.lst

clean:
	rm -f *.o *.lib

