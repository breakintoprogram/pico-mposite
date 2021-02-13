# pico-mposite
A hacked together demo for the Raspberry Pi Pico to output a composite video signal from the GPIO

#### The pico-mposite breadboard
![Breadboard](https://github.com/breakintoprogram/pico-mposite/blob/main/images/breadboard.jpeg)
Using a resistor ladder to translate a 5-bit binary number on the Pico GPIO to a voltage between 0v and 1v. The small piece of stripboard is just a bodged composite video connector and combines the output voltage from the resistor ladder and ground and presents that on a standard RCA phono socket.

#### Sample Bitmap
![Sample Bitmap](https://github.com/breakintoprogram/pico-mposite/blob/main/images/bitmap.png)

#### It works!
![It Works](https://github.com/breakintoprogram/pico-mposite/blob/main/images/video_output.jpeg)
