#include "handmade.h"

internal void GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHz)
{
    int16_t ToneVolume = 3000;
    int16_t *SampleOut = SoundBuffer->Samples;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;
    for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; SampleIndex++)
    {
        // float t = 2.0F * PI32 * (float)SoundOutput->RunningSampleIndex / (float)SoundOutput->WavePeriod;
        float SineValue = sinf(GameState->tSine);
        int16_t SampleValue = (int16_t)(SineValue * (float)ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;
        GameState->tSine += (2.0F * PI32 * 1.0F) / (float)(WavePeriod);
        if(GameState->tSine > 2.0F * PI32)
        {
            GameState->tSine -= 2.0F * PI32;
        }
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
            uint8_t Blue = (uint8_t)(X + XOffset);
            uint8_t Green = (uint8_t)(Y + YOffset);
            
            *Pixel++ = ((Green << 16) | Blue);
        }
        Row += Buffer->Pitch;
    }
}


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
            ArrayCount(Input->Controllers[0].Buttons));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);                      
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!Memory->IsInitialize)
    {
        const char *Filename = __FILE__;

        debug_read_file_result File = Memory->DEBUGPlatformReadEntireFile(Filename);
        if(File.Contents)
        {
            Memory->DEBUGPlatformWriteEntireFile("R:/test.out", File.ContentsSize, File.Contents);
            Memory->DEBUGPlatformFreeFileMemory(File.Contents);
        }
        
        GameState->ToneHz = 256;
        GameState->tSine = 0.0F;
        GameState->GreenOffset = 0;
        GameState->BlueOffset = 0;

        //TODO: This may be more appropriate to do in the platform layer
        Memory->IsInitialize = true;
    }
    for(int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        if(Controller->IsAnalog)
        {
            GameState->BlueOffset += (int)(4.0F * (Controller->StickAverageX));
            GameState->ToneHz = 256 + (int)(128.0F * (Controller->StickAverageY));   
        }
        else
        {
            if(Controller->MoveLeft.EndedDown)
            {
                GameState->BlueOffset -= 1;
            }
            if(Controller->MoveRight.EndedDown)
            {
                GameState->BlueOffset += 1;
            }   
        }

        if(Controller->ActionDown.EndedDown)
        {
            GameState->GreenOffset += 1;
        }
    }
    RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}
extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameOutputSound(GameState, SoundBuffer, GameState->ToneHz);
}
