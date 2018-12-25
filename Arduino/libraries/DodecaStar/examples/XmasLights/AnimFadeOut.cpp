#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include "AnimFadeOut.h"

int AnimFadeOutRunTime = 0;

MyAnimationState animationState[AnimCount];
uint16_t frontPixel = 0;  // the front of the loop
RgbColor frontColor;  // the color at the front of the loop

void FadeOutAnimUpdate(const AnimationParam& param)
{
    // this gets called for each animation on every time step
    // progress will start at 0.0 and end at 1.0
    // we use the blend function on the RgbColor to mix
    // color based on the progress given to us in the animation
    RgbColor updatedColor = RgbColor::LinearBlend(
        animationState[param.index].StartingColor,
        animationState[param.index].EndingColor,
        param.progress);
    // apply the color to the strip
    strip.SetPixelColor(animationState[param.index].IndexPixel,
        colorGamma.Correct(updatedColor));
}

void AnimFadeOut(const AnimationParam& param)
{
    // wait for this animation to complete,
    // static int loops = 0;
    // we are using it as a timer of sorts
    if (param.state == AnimationState_Completed)
    {
      // done, time to restart this position tracking animation/timer
      if(AnimFadeOutRunTime < 1000)
      {
        AnimFadeOutRunTime++;
        animations.RestartAnimation(param.index);
        // Serial.println(loops++);
        // pick the next pixel inline to start animating
        frontPixel = (frontPixel + 1) % PixelCount; // increment and wrap
        if (frontPixel == 0)
        {
            // we looped, lets pick a new front color
            frontColor = HslColor(random(360) / 360.0f, 1.0f, 0.25f);
        }

        uint16_t indexAnim;
        // do we have an animation available to use to animate the next front pixel?
        // if you see skipping, then either you are going to fast or need to increase
        // the number of animation channels
        if (animations.NextAvailableAnimation(&indexAnim, 1))
        {
            animationState[indexAnim].StartingColor = frontColor;
            animationState[indexAnim].EndingColor = RgbColor(0, 0, 0);
            animationState[indexAnim].IndexPixel = frontPixel;

            animations.StartAnimation(indexAnim, PixelFadeDuration, FadeOutAnimUpdate);
        }
      }
    }
}
