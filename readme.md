# Multi-Firmware Tracker

The GameTank's audio hardware is essentially a software-defined soundcard. This is great but it makes it hard to pin down for making composition tools.

Multi-Firmware Tracker is an attempt at solving that by making it extremely configurable. How extreme? Well I think I accidentally created a tiny programming languguage in the process.

Anyway this tracker models communication with the Audio Coprocessor as a collection of memory locations given names and parametric relationships with note values from the tracker sequence.

It's still very early but at least now you can load and save songs, and load and save audio firmware configurations. To an extent you should be able to take most GameTank ACP firmwares and get them to do *something*. Though to set it up you might have to do your configuring in a text editor rather than in the GUI for now.

## Usage:

After building run it with the repo root (not the build folder) as your working directory so it can pick up its default config files.

Run it with no args to open an empty song

Run it with 1 arg thats a song name and it'll open that song


## Keys:

Letters and numbers map to notes

Space toggles record

Del clears the current space and moves on

Backsp steps backwards and clears the space

/ and * on the numpad change octave

Enter starts playing the song from the top of the current pattern

## Other controls:

You can right click in the the little top left window thing to duplicate, delete, or add pattern frames.

Want to clear and start a new song? Close it and reopen it, I'm too tired to chase down memory leak city right now and properly clear all the pointers.

While we're at it don't use Open Song if you already opened a song or added anything, right now Open Song acts weirdly *additive* at least so far as instruments are concerned..