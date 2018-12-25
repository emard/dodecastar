#ifndef ANIMSEQUENCER_H
#define ANIMSEQUENCER_H

#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include "NeoPixel.h"

const uint16_t AnimSequencer_interval = 100; // ms how fast (10 Hz) it checks to start new sequence

// what is stored for state is specific to the need, in this case, the colors and
// the pixel to animate;
// basically what ever you need inside the animation update function
struct AnimSequencerState
{
    RgbColor StartingColor;  // the color the animation starts at
    RgbColor EndingColor; // the color the animation will end at
    AnimEaseFunction Easeing; // the acceleration curve it will use 
};

void AnimSequencer(const AnimationParam& param); // parent spawner

extern AnimSequencerState animSequencerState[];

#endif
