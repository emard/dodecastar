# Dodecastar

Star-shaped 3D printable object "useful" as a lamp shade
for a string of WS2812B LEDs for Xmas decoration lights.

Dodecastar is a dodecahedron based shape with
12 five-pointed stars, inspired by vintage Xmas
lights from 1960's, made of clear transparent plastic. 
Shape is modified for mounting on LED string.

Written in openscad. To visualize light reflection, it 
could be exported to "OFF" format and imported into Blender.
Example [pictures](/pic/)

![pictures](/pic/dodecastar12.png)
![pictures](/pic/render11-dodecahedron-inside.png)

# Xmas Lights Assembly

The dodecastars are intended to be mounted on a string of
[50Pcs.WS2812B Pre-wired LEDs](http://www.ebay.com/itm/50Pcs-WS2812B-Pre-wired-LED-Pixel-Module-String-Light-Full-Color-5050-RGB-5V-TZ-/201965803082?hash=item2f0619964a:g:KbEAAOSwXeJYEGiZ).
Each star consists of 2 halves, so let's print 100 of them
with transparent colorless filament (I recommend PET-G).
Printing each half takes about 7 minutes. It will appear
as icy-white. It would be ideal if it were really
transparent, but filament extrusion can't make it.

Tighten 2 halves of dodecastar gently with 2 small inox screws for 
plastic with round philips head, thread dia: 2.2 mm, thread length: 6 mm.

Carefully handle new LED strip. Soldered connections cannot 
stand any mechanical force. Only mounted dodecastars provide
mechanical strength to whole LED strip.

Solder the USB A male connector nn Din side of WS2812B module.
I strongly recommend pinout compatible with normal USB,
so that it can be accidentaly plugged into PC without
any damage, but it will not light up yet because WS2812B 
needs different signal protocol.

    WS2812B    USB-A
    -------    -----
    GND        GND
    Din        D+
               D-
    +5V        +5V

# Hardware

Various [Arduino](http://arduino.cc) compatible hardware 
should work, even ATTINY85.
I use ESP8266 NodeMCU 0.9 with GPIO2 connected to LED Din
(GPIO2 is arduino Pin 2, on NodeMCU is labeled "D4")
Connect GND and 5V from the board to LED strip.
No resistors or capacitors needed for NodeMCU.

# Software

Install [Arduino](http://arduino.cc), and if required
the core support for your board, for ESP8266 it is

Download
[Adafruit NeoPixel library](https://github.com/adafruit/Adafruit_NeoPixel)
And copy it to ~/Arduino/libraries/Adafruit_NeoPixel

Open Examples->Adafruit NeoPixel->Strandtest
Edit source to set PIN connected to Din 
(pin 2 in my case) and number of LEDs (50).

    #define PIN 2 // Pin 2 is NodeMCU pin D4
    Adafruit_NeoPixel strip = Adafruit_NeoPixel(50, PIN, NEO_GRB + NEO_KHZ800);

Compile and upload, whole LED string will light up in
various colors!
