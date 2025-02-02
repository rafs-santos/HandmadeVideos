#include "handmade.h"

internal void GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz)
{
    local_persist float tSine;
    int16_t ToneVolume = 3000;
    int16_t *SampleOut = SoundBuffer->Samples;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;
    for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; SampleIndex++)
    {
        // float t = 2.0F * PI32 * (float)SoundOutput->RunningSampleIndex / (float)SoundOutput->WavePeriod;
        float SineValue = sinf(tSine);
        int16_t SampleValue = (int16_t)(SineValue * (float)ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;
        tSine += (2.0F * PI32 * 1.0F) / (float)(WavePeriod);
    }

}

internal void RenderWeirdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
    
    // Buffer->Pitch = Buffer->Width*Buffer->BytesPerPixel;

    uint8_t *Row = (uint8_t *)Buffer->Memory;
    for(int Y = 0; Y < Buffer->Height; ++Y){
        uint32_t *Pixel = (uint32_t *)Row;
        for(int X = 0; X < Buffer->Width; X++){
            /*
                Pixel in memory BB GG RR XX

            */
            uint8_t Blue = (X + XOffset);
            uint8_t Green = (Y + YOffset);
            
            *Pixel++ = ((Green << 8) | Blue);
        }
        Row += Buffer->Pitch;
    }
}


internal void GameUpdateAndRender(game_offscreen_buffer *Buffer,  int XOffset, int YOffset,
                                game_sound_output_buffer *SoundBuffer, int ToneHz)
{
    // TODO: Allow samples offsets here for more robust platform options
    GameOutputSound(SoundBuffer, ToneHz);
    RenderWeirdGradient(Buffer, XOffset, YOffset);
}