#ifndef ANIMFADEOUT_H
#define ANIMFADEOUT_H

#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include "NeoPixel.h"

const uint16_t PixelFadeDuration = 300; // third of a second
// one second divide by the number of pixels = loop once a second
const uint16_t AnimFadeOut_NextPixelMoveDuration = 1000 / PixelCount; // how fast we move through the pixels

extern int AnimFadeOutRunTime;

// what is stored for state is specific to the need, in this case, the colors and
// the pixel to animate;
// basically what ever you need inside the animation update function
struct MyAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
    uint16_t IndexPixel; // which pixel this animation is effecting
};

void AnimFadeOut(const AnimationParam& param); // parent spawner
void FadeOutAnimUpdate(const AnimationParam& param); // child worker

extern MyAnimationState animationState[];
extern uint16_t frontPixel;
extern RgbColor frontColor;

#endif
