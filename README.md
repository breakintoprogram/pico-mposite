# pico-mposite

### Summary
Hardware and firmware for the Raspberry Pi Pico to add a composite video output using GPIO.

![Splash Screen](/images/demo_splash_colour.jpeg)

### Why am I doing this?
The code has evolved from a demo I built when the Pico was first released. I wanted a simple project that combined the use of DMA and the Pico PIO cores. Getting the Pico to output a composite video signal was a good candidate for that.

I decided the project had legs so updated the original mono version to add colour. This required a rewrite of my original demo and new hardware, yet both versions can still be built from this code. And both versions support a simple serial graphics terminal for homebrew 8-bit systems like my BSX and the RC2014. The capabilities of that may be expanded in future.

And I've also created PCB layouts for both versions. Files are available for the mono version, with the colour version to follow once I've tested it.

### Versions
There are two versions of the hardware and firmware

#### Mono
- 16 shades of grey

Uses a single resistor ladder to convert a 5-bit binary number on the Pico GPIO to a voltage between 0v and 1v. This is used to directly drive the composite video signal.

#### Colour
- 256 colours (322 RGB)

Uses three resistor ladders and an AD724 PAL/NTSC encoder chip.

Both monochrome and colour versions of the circut support resolutions of 256x192, 320x192 and 640x192.

For more details, see [my blog post detailing the build](http://www.breakintoprogram.co.uk/projects/pico/composite-video-on-the-raspberry-pi-pico).

### Hardware

#### Build on a breadboard
The breadboard files can be found in the folder [/hardware/breadboard/](/hardware/breadboard/)

#### Build on a PCB
PCB files will be added soon

### Firmware
The code to use two PIO state machines. The first state machine creates a blank PAL(ish) video signal using a handful of 32-byte hard-coded lookup tables fetched via DMA, each pulse being 2us wide. At the point where pixel data needs to be injected into that signal a second state machine kicks in and writes that data out to the GPIO pins at a higher frequency, whilst the first state machine executes NOPs.

This allows for the horizontal video resolution to be tweaked independantly of the sync pulses, and is required for the colour version.

The firmware includes a handful of extras to get folk started on projects based upon this; some graphics primitives, and a handful of rolling demos.

The graphics primitives include:

- Plot and Line
- Circle, Triangle and Polygon (Wireframe and Filled)
- Print
- Clear Screen, Vsync and Border
- Scroll and Blit

There is also a terminal mode. This requires a serial connection to the UART on pins 12 and 13 of the Pico. Remember the Pico is not 5V tolerant; the sample circuits uses a resistor divider circuit to drop a 5V TTL serial connection to 3.3V. This is very much work-in-progress.

### Configuring for compilation
In config.h there are a couple of compilation options:
- opt_colour:
  - Set to 0 to build firmware for the mono version
  - Set to 1 to build firmware for the colour version
- opt_terminal
  - Set to 0 to just run rolling demos
  - Set to 1 to build the serial terminal

### Building
Make sure that you have set an environment variable to the Pico SDK, substituting the path with the location of the SDK files on your computer.
```shell
export PICO_SDK_PATH=/home/dev/pico/pico-sdk-2.0.0
```

To build, execute these commands inside the `build` folder.
```shell
cmake ..
make
```
This should create the file `pico-mposite.uf2` that you can upload to your Pico.