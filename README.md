# wii-gc-adapter

Based off of wii-u-gc-adapter by ToadKing, wii-gc-adapter is a
mod for Super Smash Bros. Brawl that allows you to use
gamecube controllers, via the official Nintendo Wii u Gamecube
Controller Adapter. Implementation is based on Gecko OS.
(Riivolution, previously)

Non-USA versions aren't _planned_, but you never know!

Of course, note that it should be possible to port
this to games besides SSBB with relative ease.
(Haven't tested this yet, but... _theoretically..._)

For the bug tracker and whatnot, see
[the github repo.](https://github.com/wilm0x42/wii-gc-adapter-inject)

## Building and usage

### Building
**This is _only_ designed to build on Unix-like systems!**
Theoretically, it _might_ build on windows with cygwin,
but I don't know, and really couldn't care less.

First, you just need to `make` the buildtools:
`cd buildtools; make; cd ..`

Use `make` for building:
 * `make` to build complete mod.
 * `make clean` to delete generated files.
 * `make sd.raw` to generate an SD card image compatible with dolphin.
 
Requires: (Please let me know if I'm forgetting something here)

[devkitPPC](https://devkitpro.org/) and libogc for compilation,
Mtools for sd.raw (`sudo apt-get install mtools`).

### Usage
To use, copy the contents of the generated `gecko` folder to the
root of your SD card, and start [Gecko OS](http://wiibrew.org/wiki/Gecko_OS)
on your wii.

:warning: IMPORTANT: Make sure "SD File Patcher" is enabled
in the options of Gecko OS.

If the game crashes after or during the initial loading screen, just
turn off your wii by holding down the power button, and try again.

### Usage with other mods (e.g. Project M)

This is compatible with Project M, and probably most if not all other mods.

1. Locate the gecko code this mod uses in `wii-gc-adapter-gct.txt`
2. Add this code to your prexisting codeset using GCTEdit or something.
3. Place `RSBE01.gct` and `RSBE01.gpf` into the `codes` and `patch` folders,
   respectively. (These are on your SD card)

## Thanks

None of this would be possible without:

Everyone involved in devkitpro/devkitPPC/libogc
(WinterMute, Team Twiizers, etc.),
[wii-u-gc-adapter](https://github.com/ToadKing/wii-u-gc-adapter), by ToadKing.
[Wiibrew wiki](http://wiibrew.org), and all the contributors thereof,
Dolphin Emulator, for it's invaluable debugging environment,
Smashboards, for archiving random, VERY useful info about Brawl hacking,
The folks behind [Riivolution](http://rvlution.net),
Gecko OS, plus all of the brilliant hackers on geckocodes.org,
and all the other's who I'm forgetting.