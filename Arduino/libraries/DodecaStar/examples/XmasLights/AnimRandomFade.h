#ifndef ANIMRANDOMFADE_H
#define ANIMRANDOMFADE_H

#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include "NeoPixel.h"

const uint16_t AnimRandomFade_NextPixelMoveDuration = 5000 / PixelCount; // ms how fast we check for new pixel

extern int AnimRandomFadeRunTime;

// what is stored for state is specific to the need, in this case, the colors and
// the pixel to animate;
// basically what ever you need inside the animation update function
struct AnimRandomFadeState
{
    RgbColor StartingColor;  // the color the animation starts at
    RgbColor EndingColor; // the color the animation will end at
    AnimEaseFunction Easeing; // the acceleration curve it will use 
};

void AnimRandomFade(const AnimationParam& param); // parent spawner
void RandomFadeUpdate(const AnimationParam& param); // child worker

extern AnimRandomFadeState animRandomFadeState[];

#endif
