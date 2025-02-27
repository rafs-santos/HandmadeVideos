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
            uint8_t Blue = (uint8_t)(X + XOffset);
            uint8_t Green = (uint8_t)(Y + YOffset);
            
            *Pixel++ = ((Green << 8) | Blue);
        }
        Row += Buffer->Pitch;
    }
}

// internal game_state *GameStartup(void)
// {
//     game_state *GameState = new game_state;
//     if(GameState)
//     {
//         GameState->BlueOffset = 0;
//         GameState->GreenOffset = 0;
//         GameState->ToneHz = 256;
//     }

//     return GameState;
// }

// internal void GameShutDown(game_state *GameState)
// {
//     delete GameState;
// }

internal void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer,
                                  game_sound_output_buffer *SoundBuffer)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
            ArrayCount(Input->Controllers[0].Buttons));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);                      
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!Memory->IsInitialize)
    {
        const char *Filename = __FILE__;

        debug_read_file_result File = DEBUGPlatformReadEntireFile(Filename);
        if(File.Contents)
        {
            DEBUGPlatformWriteEntireFile("R:/test.out", File.ContentsSize, File.Contents);
            DEBUGPlatformFreeFileMemory(File.Contents);
        }
        
        GameState->ToneHz = 256;
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
    
    // TODO: Allow samples offsets here for more robust platform options
    GameOutputSound(SoundBuffer, GameState->ToneHz);
    RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}