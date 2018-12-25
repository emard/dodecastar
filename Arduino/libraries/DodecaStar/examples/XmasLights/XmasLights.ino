#include <ESP8266WiFi.h>
#include "FS.h"
#include <WiFiClient.h>
#include <TimeLib.h>
#include <NtpClientLib.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include "DodecaStar.h"
#include <Hash.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include "AnimSequencer.h"
#include "AnimFadeOut.h"
#include "AnimRandomFade.h"
#include "SetRandomSeed.h"

NeoGamma<NeoGammaTableMethod> colorGamma; // for any fade animations, best to correct gamma

//NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
// For Esp8266, the Pin is omitted and it uses GPIO3 due to DMA hardware use.
// There are other Esp8266 alternative methods that provide more pin options, but also have
// other side effects.
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount);
//
// NeoEsp8266Uart800KbpsMethod uses GPI02 instead

NeoPixelAnimator animations(AnimCount, NEO_MILLISECONDS); // NeoPixel animation management object
// since the normal animation time range is only about 65 seconds, by passing timescale value
// to the NeoPixelAnimator constructor we can increase the time range, but we also increase
// the time between the animation updates.   
// NEO_CENTISECONDS will update the animations every 100th of a second rather than the default
// of a 1000th of a second, but the time range will now extend from about 65 seconds to about
// 10.9 minutes.  But you must remember that the values passed to StartAnimations are now 
// in centiseconds.
//
// Possible values from 1 to 32768, and there some helpful constants defined as...
// NEO_MILLISECONDS        1    // ~65 seconds max duration, ms updates
// NEO_CENTISECONDS       10    // ~10.9 minutes max duration, centisecond updates
// NEO_DECISECONDS       100    // ~1.8 hours max duration, decisecond updates
// NEO_SECONDS          1000    // ~18.2 hours max duration, second updates
// NEO_DECASECONDS     10000    // ~7.5 days, 10 second updates

void setup() {
    SPIFFS.begin(); // Not really needed, checked inside library and started if needed
    // WiFi is started inside library
    ESPHTTPServer.begin(&SPIFFS);
    /* add setup code here */
    strip.Begin();
    strip.Show();
    SetRandomSeed();
    // todo: can strip pixel count be changed at runtime?
    // ESPHTTPServer.setPixelCount(PixelCount);
    // we use the index 0 animation to run a "process" which starts other animations
    animations.StartAnimation(0, AnimSequencer_interval, AnimSequencer);
    // animations.StartAnimation(0, AnimFadeOut_NextPixelMoveDuration, AnimFadeOut);
    // animations.StartAnimation(0, AnimRandomFade_NextPixelMoveDuration, AnimRandomFade);
}

void loop() {
    /* add main program code here */
    // this is all that is needed to keep it running
    // and avoiding using delay() is always a good thing for
    // any timing related routines
    if(ESPHTTPServer.getDodecaStarMode() == DSM_animate)
      animations.UpdateAnimations();
    if(ESPHTTPServer.getDodecaStarMode() == DSM_pixel)
    {
      for(int i = 0; i < ESPHTTPServer.getPixelCount(); i++)
        strip.SetPixelColor(i,
          RgbColor((HtmlColor) ESPHTTPServer.getPixelValue(i))
        );
    }
    strip.Show();
    // DO NOT REMOVE. Attend OTA update from Arduino IDE
    ESPHTTPServer.handle();	
}
