Salut
=====

*a Simple Audio LPC and UDP based Talk program*

Description
-----------

* `duplex` duplex LPC encoding/ decoding demo.

* `salut` simple VOIP program based on the previous LPC codec, relies on UNIX
 sockets for the network and the GTK library for the basic GUI.

* some portaudio based demo programs can also be found under the `tests/`
 directory

Dependencies
------------

* [PortAudio](http://portaudio.com/)
* [GTK3+](http://www.gtk.org/)
* [liquid-dsp](http://liquidsdr.org/)

Compilation
-----------

* use `make` to compile both binaries or `make target` for a specific target

*_Disclaimer_*: both programs have only been tested on GNU/Linux distributions
(Arch Linux and Fedora 21) and should not be expected to run (or even build) on
other platforms.
