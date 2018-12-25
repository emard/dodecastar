#ifndef NEOPIXEL_H
#define NEOPIXEL_H
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

enum
{ 
  PixelCount = 200, // set this to the number of pixels in your strip
  PixelPin = 3, // set this to the correct pin, ignored for ESP8266
  AnimCount = 400 // number of parallel processes, 2x more than pixels should be enough
};

extern NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip; // (PixelCount);
extern NeoGamma<NeoGammaTableMethod> colorGamma;
extern NeoPixelAnimator animations; // (AnimCount);

#endif
