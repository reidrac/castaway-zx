# Castaway

This is the source code of [Castaway](https://www.usebox.net/jjm/castaway/), a
game for the ZX Spectrum 48K (or later).

I'm sharing it as an historic curiosity and hoping that it may be interesting;
because the code quality isn't great (my excuse is that I was learning, what is
yours?), and some of the dependencies may require a bit too much effort to
compile the game.

You will need:

- A POSIX environment (Linux is perfect)
- GCC, GNU Make, Python 2
- Z88DK v1.9 (other versions may or may not work)
- sp1.lib from Z88DK

If you want to build this code with a more recent version of Z88DK, please
check [z88dk-nightly branch](https://github.com/reidrac/castaway-zx/tree/z88dk-nightly).
It has been tested on MacOS only, but it may be easier than getting Z88DK v1.9
to compile (I'm told, I didn't have any issue in Debian).

Then cross your fingers and run `make`.

It should end with:
```
***
    Max: 26936 bytes
Current: 26875 bytes (umain.bin)
   Left: 61 bytes
***
```

And `castawy.tap` should be ready to load in an emulator.

I provide this source code "as is" without any support.

## License

The source code of the game is licensed GPL 3.0, the assets are [CC-BY-SA](https://creativecommons.org/licenses/by-sa/2.0/).

The tools/libraries included that I don't own have their own copyright notices
and license (some are public domain, others are open source).

