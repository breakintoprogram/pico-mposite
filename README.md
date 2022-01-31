# pico-mposite
A hacked together demo for the Raspberry Pi Pico to output a composite video signal from the GPIO

#### The pico-mposite breadboard
![Breadboard](https://github.com/breakintoprogram/pico-mposite/blob/main/images/breadboard.jpeg)
Using a resistor ladder to translate a 5-bit binary number on the Pico GPIO to a voltage between 0v and 1v. The small piece of stripboard is just a bodged composite video connector and combines the output voltage from the resistor ladder and ground and presents that on a standard RCA phono socket. I didn't have the exact resistor values to hand so used the nearest values available which were 470Ω, 220Ω and 100Ω.

#### The code
The code has been updated to use two PIO state machines. The first state machine creates a blank PAL(ish) video signal using a handful of 32-byte hard-coded lookup tables fetched via DMA. At the point where pixel data needs to be injected into that scaffold, a second state machine kicks in and writes that data out to the GPIO pins, whilst the first state machine executes NOPs.

This is an improvement on the original code that generated the video sync data inline with the pixel data, and required the video pulses to be defined
in pixel widths, rather than 2us pulse widths. It allows for the video resolution to be tweaked more easily, and is a step towards the next version of this code that will output a colour PAL signal.

#### Sample Bitmap
![Sample Bitmap](https://github.com/breakintoprogram/pico-mposite/blob/main/images/bitmap.png)

#### It works!
![It Works](https://github.com/breakintoprogram/pico-mposite/blob/main/images/video_output.jpeg)
