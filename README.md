# wii-gc-adapter

Based off of wii-u-gc-adapter by ToadKing, wii-gc-adapter is a
mod for Super Smash Bros. Brawl that should, when it's completed,
allow you to use gamecube controllers, via the official
Nintendo Wii u Gamecube Controller Adapter. Implementation
is based on riivolution.

Non-USA versions aren't _planned_, but you never know!

Of course, note that it should be possible to port
this to games besides SSBB with relative ease.

## Building and usage

**This _only_ designed to build on Unix-like systems!**
Theoretically, it _might_ build on windows with cygwin,
but I don't know, and really couldn't care less.

Use `make` for building:
 * `make` to build complete mod.
 * `make clean` to delete generated files.
 * `make sd.raw` to generate an SD card image compatible with dolphin.
 
Requires: (Please let me know if I'm forgetting something here)

[devkitPPC](https://devkitpro.org/) and libogc for compilation,
Mtools for sd.raw (`sudo apt-get install mtools`).

If make complains about "generateBl" in any way, try
recompiling generateBl, within `buildtools`:
`rm generateBl; gcc -o generateBl ./generateBl.c`


## Thanks

None of this would be possible without:

Everyone involved in devkitpro/devkitPPC/libogc
(WinterMute, Team Twiizers, etc.),
[wii-u-gc-adapter](https://github.com/ToadKing/wii-u-gc-adapter), by ToadKing.
[Wiibrew wiki](http://wiibrew.org), and all the contributors thereof,
Dolphin Emulator, for it's invaluable debugging environment,
Smashboards, for archiving random, VERY useful info about Brawl hacking,
The folks behind [Riivolution](http://rvlution.net),
Gecko OS, and all of the brilliant hackers on geckocodes.org,
and all the other's who I'm forgetting.