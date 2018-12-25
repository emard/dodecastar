// NeoPixelAnimation
// This example will randomly pick a new color for each pixel and animate
// the current color to the new color over a random small amount of time, using
// a randomly selected animation curve.
// It will repeat this process once all pixels have finished the animation
// 
// This will demonstrate the use of the NeoPixelAnimator extended time feature.
// This feature allows for different time scales to be used, allowing slow extended
// animations to be created.
// 
// This will demonstrate the use of the NeoEase animation ease methods; that provide
// simulated acceleration to the animations.
//
// It also includes platform specific code for Esp8266 that demonstrates easy
// animation state and function definition inline.  This is not available on AVR
// Arduinos; but the AVR compatible code is also included for comparison.
//
// The example includes some serial output that you can follow along with as it 
// does the animation.
//

#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include "AnimRandomFade.h"

int AnimRandomFadeRunTime = 0;

#ifdef ARDUINO_ARCH_AVR
// for AVR, you need to manage the state due to lack of STL/compiler support
// for Esp8266 you can define the function using a lambda and state is created for you
// see below for an example
struct MyAnimationState
{
    RgbColor StartingColor;  // the color the animation starts at
    RgbColor EndingColor; // the color the animation will end at
    AnimEaseFunction Easeing; // the acceleration curve it will use 
};

MyAnimationState animationState[PixelCount];
// one entry per pixel to match the animation timing manager


void AnimUpdate(const AnimationParam& param)
{
    // first apply an easing (curve) to the animation
    // this simulates acceleration to the effect
    float progress = animationState[param.index].Easeing(param.progress);

    // this gets called for each animation on every time step
    // progress will start at 0.0 and end at 1.0
    // we use the blend function on the RgbColor to mix
    // color based on the progress given to us in the animation
    RgbColor updatedColor = RgbColor::LinearBlend(
        animationState[param.index].StartingColor,
        animationState[param.index].EndingColor,
        progress);
    // apply the color to the strip
    strip.SetPixelColor(param.index, updatedColor);
}
#endif


void RandomFadePixel(uint16_t indexAnim, uint16_t pixel)
{
    // setup some animation
        const uint8_t peak = 128;

        // pick a random duration of the animation for this pixel
        // since values are centiseconds, the range is 1 - 4 seconds
        uint16_t time = random(100, 400);

        // each animation starts with the color that was present
        RgbColor originalColor = strip.GetPixelColor(pixel);
        // and ends with a random color
        RgbColor targetColor = RgbColor(random(peak), random(peak), random(peak));
        // with the random ease function
        AnimEaseFunction easing;

        switch (random(3))
        {
        case 0:
            easing = NeoEase::CubicIn;
            break;
        case 1:
            easing = NeoEase::CubicOut;
            break;
        case 2:
            easing = NeoEase::QuadraticInOut;
            break;
        }

#ifdef ARDUINO_ARCH_AVR
        // each animation starts with the color that was present
        animationState[pixel].StartingColor = originalColor;
        // and ends with a random color
        animationState[pixel].EndingColor = targetColor;
        // using the specific curve
        animationState[pixel].Easeing = easing;

        // now use the animation state we just calculated and start the animation
        // which will continue to run and call the update function until it completes
        animations.StartAnimation(pixel, time, AnimUpdate);
#else
        // we must supply a function that will define the animation, in this example
        // we are using "lambda expression" to define the function inline, which gives
        // us an easy way to "capture" the originalColor and targetColor for the call back.
        //
        // this function will get called back when ever the animation needs to change
        // the state of the pixel, it will provide a animation progress value
        // from 0.0 (start of animation) to 1.0 (end of animation)
        //
        // we use this progress value to define how we want to animate in this case
        // we call RgbColor::LinearBlend which will return a color blended between
        // the values given, by the amount passed, hich is also a float value from 0.0-1.0.
        // then we set the color.
        //
        // There is no need for the MyAnimationState struct as the compiler takes care
        // of those details for us
        AnimUpdateCallback animUpdate = [=](const AnimationParam& param)
        {
            // progress will start at 0.0 and end at 1.0
            // we convert to the curve we want
            float progress = easing(param.progress);

            // use the curve value to apply to the animation
            RgbColor updatedColor = RgbColor::LinearBlend(originalColor, targetColor, progress);
            strip.SetPixelColor(pixel, updatedColor);
        };

        // now use the animation properties we just calculated and start the animation
        // which will continue to run and call the update function until it completes
        animations.StartAnimation(indexAnim, time, animUpdate);
#endif
}

void AnimRandomFade(const AnimationParam& param)
{
    // wait for this animation to complete and restart it
    // we are using it as a timer to spawn annother animation
    // that actually does the pixel change
    static uint16_t loop = 0;
    if (param.state == AnimationState_Completed)
    {
      // done, time to restart this position tracking animation/timer
      // if runtime of this animation hasn't expired
      if(AnimRandomFadeRunTime < 1000)
      {
        AnimRandomFadeRunTime++;
        animations.RestartAnimation(param.index);
        int retry;

        for(retry = 0; retry < PixelCount/5; retry++)
        {
          uint16_t Pixel = random(PixelCount);
          // pick random pixel to start animating
          uint16_t indexAnim = Pixel + param.index; // calculate its associated animation process number
          // check if it is not already animating
          if(!animations.IsAnimationActive(indexAnim))
          {
            // not active - set new animation for this pixel
            RandomFadePixel(indexAnim, Pixel);
            return;
          }
        }
        // Serial.println("iteration " + String(loop) + ": " + String(retry) + " pixels tried, all busy");
      }
    }
}
