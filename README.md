# pico-mposite
A hacked together demo for the Raspberry Pi Pico to output a composite video signal from the GPIO

### The pico-mposite breadboard
![Breadboard](https://github.com/breakintoprogram/pico-mposite/blob/main/images/breadboard.jpeg)
There are two variants of the breadboard, a simple one that uses a resistor ladder to translate a 5-bit binary number on the Pico GPIO to a voltage between 0v and 1v, that generates an image with 16 shades of grey, and a more complicated one that uses 3 resistor ladders and an AD724 PAL/NTSC encoder chip to generate a 256 colour bitmap with 1 byte per pixel (332 RGB). Both schematics are included in the fritzing folder.

Both monochrome and colour versions of the circut support resolutions of 256x192, 320x192 and 640x192.

For more details, see [my blog post detailing the build](http://www.breakintoprogram.co.uk/projects/pico/composite-video-on-the-raspberry-pi-pico).

### The code
The original demo used a single state machine to output the video sync pulses and pixel data. This required the sync pulses to be defined
at the same frequency as the pixels.

I've updated the code to use two PIO state machines. The first state machine creates a blank PAL(ish) video signal using a handful of 32-byte hard-coded lookup tables fetched via DMA, each pulse being 2us wide. At the point where pixel data needs to be injected into that scaffold, a second state machine kicks in and writes that data out to the GPIO pins at a higher frequency, whilst the first state machine executes NOPs.

This is an improvement on the original code. It allows for the video resolution to be tweaked independantly of the sync pulses, and is required for the colour version.

In addition, I've done some housekeeping on the code, refactoring it to make it more usable. I've also includes a handful of extras to get folk started on projects based upon this; some graphics primitives, and a handful of rolling demos.

The graphics primitives include:

- Plot and Line
- Circle, Triangle and Polygon (Wireframe and Filled)
- Print
- Clear Screen, Vsync and Border
- Scroll and Blit

There is also a terminal mode. This requires a serial connection to the UART on pins 12 and 13 of the Pico. Remember the Pico is not 5V tolerant; the sample circuits uses a resistor divider circuit to drop a 5V TTL serial connection to 3.3V.

### Configuring for compilation
In config.h there are a couple of compilation options:
- opt_colour: Set to 0 to compile for the monochrome version, 1 to compile for the colour version (requires a different circuit)
- opt_terminal: Set to 0 to just run rolling demos, 1 to enter terminal mode

### Building
To build, execute these commands inside the `build` folder.
```shell
cmake ..
make
```
This should result in the file `pico-mposite.uf2`, which you can upload to your Pico.

### Why am I doing this?
The code has evolved from a demo I built last year to demonstrate using the Pico PIO to output a composite video signal. I've decided the project has legs, and have decided to update it with the intention of making a colour version, and adding more support code for any developers that want to use this as a reference for their own projects.

I'm also investigating the possibility of using it as a serial graphics terminal for homebrew 8-bit systems like my BSX and the RC2014.

![Splash Screen](https://github.com/breakintoprogram/pico-mposite/blob/main/images/demo_splash_colour.jpeg)
