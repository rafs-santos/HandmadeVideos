#ifndef HANDMADE_H
#define HANDMADE_H
/*
    NOTE: Services that the game provides to the platform layer.
*/

// timing, controller/keyboard input, bitmap buffer to use, sound buffer to use

struct game_offscreen_buffer
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct game_sound_output_buffer
{
    int SamplesPerSecond;
    int SampleCount;
    int16_t *Samples;
};

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset, int YOffset,
                                game_sound_output_buffer *SoundBuffer);


/*
    NOTE: Services that the platform layer provides to the platform layer.
*/


#endif