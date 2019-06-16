# Wii-GCA-Inject

Based off of wii-u-gc-adapter by ToadKing, Wii-GCA-Inject (formerly known as wii-gc-adapter) is a
mod for Super Smash Bros. Brawl that allows you to use
gamecube controllers, via the official Nintendo Wii u Gamecube
Controller Adapter. It's currently working using a .gpf file,
compatible only with Gecko OS, but a standard gecko code version
on the way, hopefully Coming Soon™.

All controller functionality, including rumble and hotplugging, is supported.
(That is, hotplugging of both controllers AND adapters! :D)

Non-USA versions aren't _planned_, but you never know!

Of course, note that it should be possible to port
this to games besides SSBB with relative ease.
(Haven't tested this yet, but... _theoretically..._)

For the bug tracker and whatnot, see
[the github repo.](https://github.com/wilm0x42/wii-gc-adapter-inject)
Also, there's a [thread on Smashboards.](https://smashboards.com/threads/wii-gca-inject-pm-on-wii-u-with-gamecube-adapter-beta.453431)

## Current State of Development (June 2019)
(Just gonna retrofit the wall of text from [#5](https://github.com/wilm0x42/wii-gc-adapter-inject/issues/5))

So, I sorta hit a brick wall last year in debugging this stuff and ultimately
got a bit too burnt out on it. (debugging stuff in a dormant hacking scene
can take a lot out of you :P) However, I'm now returning to it, and starting
to retrace my last steps.

Where things left off, the overall functionality was pretty much finished,
only needing final polish, except for the one big issue of not working
with USB loaders. Turns out, USB loaders make the USB interface a LOT
harder to work with in a hack like this, and supporting both USB loaders
AND official disc makes things even harder. The main issue that halted
everything was that the hack would be working fine on dolphin emulator
in every way, but it would crash on console, with no good method of
debugging, since I don't have a USB gecko :/ (In the unlikely chance that
you're reading this and you have one that I can buy from you, PLEASE get in touch. ;)

So yeah, sorry for the unannounced year-long hiatus :P

Going forward, I'm hoping I can find some way to hook into the game, on console,
while it's running, and poke around at the USB interface to see if I can coax
the right functionality out of it. (Normally the easiest/best way to communicate with
the wii from a computer, other than a USB gecko, would be the USB
ports themselves. But alas, that's the very subject of this debugging. :facepalm:)

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
 * `make sd.raw` to generate an SD card image compatible with dolphin. (_after_ a normal build)
 
Requires: (Please let me know if I'm forgetting something here)

[devkitPPC](https://devkitpro.org/) and libogc for compilation,
Mtools for sd.raw (`sudo apt-get install mtools`).

### Usage
To use, copy the generated `patch` folder to the
root of your SD card, and start [Gecko OS](http://wiibrew.org/wiki/Gecko_OS)
on your wii.

:warning: **IMPORTANT**: Make sure "SD File Patcher" is enabled
in the options of Gecko OS.

In case you're wondering, this does in fact use a feature of Gecko OS,
[Gecko Patch Files](https://gist.github.com/wilm0x42/2382af4e200a6bb076c91c56813aba54),
that you may have never encountered before. (I know _I_ haven't.)

This should work on Gecko OS versions as old as 1.8, theoretically.
However, it's only been tested on Gecko OS 1.9.3.1

### Use with other mods (e.g. Project M)

This is compatible with Project M, and probably most if not all other mods! :D

Simply put the `patch` folder on the SD card, just like normal, and launch the game
with Gecko OS, just like normal, but with your mod(s) of choice installed as well.
(Don't forget to enable "SD File Patcher" in Gecko OS)

Unfortunately, the debug menu in Project M will not function with these hacks.
This is because of the way Project M's debug codes get controller input; it's
not feasible to support the full functionality of PM's debug tools.

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