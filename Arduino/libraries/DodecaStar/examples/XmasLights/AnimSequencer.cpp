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
#include "AnimSequencer.h"
#include "AnimRandomFade.h"
#include "AnimFadeOut.h"


void AnimSequencer(const AnimationParam& param)
{
    // wait for this animation to complete and restart it
    // we are using it as a timer to spawn annother animation
    // that actually does the pixel change
    static uint16_t sequence = 0;
    if (param.state == AnimationState_Completed)
    {
        // done, time to restart this position tracking animation/timer
        animations.RestartAnimation(param.index);
        uint16_t indexAnim = 1 + param.index; // calculate its associated animation process number
        // check if it is not already animating
        if(!animations.IsAnimationActive(indexAnim))
        {
            // not active - run new animation sequence
            if(sequence == 0)
            {
              AnimRandomFadeRunTime = 0;
              animations.StartAnimation(indexAnim, AnimRandomFade_NextPixelMoveDuration, AnimRandomFade);
            }
            if(sequence == 1)
            {
              AnimFadeOutRunTime = 0;
              animations.StartAnimation(indexAnim, AnimFadeOut_NextPixelMoveDuration, AnimFadeOut);
            }
            sequence++; // next time choose next one
            if(sequence >= 2) // limit
              sequence = 0;
            return;
        }
    }
}
