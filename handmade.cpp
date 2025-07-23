#include "handmade.h"

internal void GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHz)
{
    int16_t ToneVolume = 3000;
    int16_t *SampleOut = SoundBuffer->Samples;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;
    for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; SampleIndex++)
    {
#if 0
        float SineValue = sinf(GameState->tSine);

        int16_t SampleValue = (int16_t)(SineValue * (float)ToneVolume);
#else
        int16_t SampleValue = 0;
#endif
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;
#if 0
        GameState->tSine += (2.0F * PI32 * 1.0F) / (float)(WavePeriod);
        if(GameState->tSine > 2.0F * PI32)
        {
            GameState->tSine -= 2.0F * PI32;
        }
#endif
    }

}

internal int32_t
RoundReal32ToInt32(real32 Real32)
{
    //TODO: Intrinsic???
    int32_t Result = (int32_t)(Real32 + 0.5F);
    return Result;
}
internal uint32_t
RoundReal32ToUInt32(real32 Real32)
{
    //TODO: Intrinsic???
    uint32_t Result = (uint32_t)(Real32 + 0.5F);
    return Result;
}

internal void
DrawRectangle(game_offscreen_buffer *Buffer,
              real32 RealMinX, real32 RealMinY, real32 RealMaxX, real32 RealMaxY,
              real32 R, real32 G, real32 B)
{

    int32_t MinX = RoundReal32ToInt32(RealMinX);
    int32_t MinY = RoundReal32ToInt32(RealMinY);
    int32_t MaxX = RoundReal32ToInt32(RealMaxX);
    int32_t MaxY = RoundReal32ToInt32(RealMaxY);

    if(MinX < 0)
    {
        MinX = 0;
    }
    if(MinY < 0)
    {
        MinY = 0;
    }

    if(MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }
    if(MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    uint32_t Color = ((RoundReal32ToUInt32(R * 255.0F) << 16) |
                     (RoundReal32ToUInt32(G * 255.0F) << 8)  |
                     (RoundReal32ToUInt32(B * 255.0F) << 0));

    //TODO: Colors
    uint8_t *Row = ((uint8_t *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);
    
    for(int Y = MinY; Y < MaxY; ++Y)
    {
        uint32_t *Pixel = (uint32_t *)Row;
        for(int X = MinX; X < MaxX; ++X)
        {
            *Pixel++ = Color;
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
        //TODO: This may be more appropriate to do in the platform layer
        Memory->IsInitialize = true;
    }
    for(int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        if(Controller->IsAnalog)
        {
            //NOTE: Use digital movement
        }
        else
        {
            //NOTE: Use digital movement 
            real32 dPlayerX = 0.0F;
            real32 dPlayerY = 0.0F;
            if (Controller->MoveUp.EndedDown) {
                dPlayerY = -1.0F;
            }
            if (Controller->MoveDown.EndedDown) {
                dPlayerY = 1.0F;
            }
            if (Controller->MoveLeft.EndedDown) {
                dPlayerX = -1.0F;
            }
            if (Controller->MoveRight.EndedDown) {
                dPlayerX = 1.0F;
            }
            GameState->PlayerX += Input->dtForFrame * dPlayerX;
            GameState->PlayerY += Input->dtForFrame * dPlayerY;
        }
    }

    uint32_t TileMap[9][17] = 
    {
        {1, 1, 1, 1,    1, 1, 1, 1,     0, 1, 1, 1,     1, 1, 1, 1, 1},
        {1, 1, 0, 0,    0, 1, 0, 0,     0, 0, 0, 0,     0, 1, 0, 0, 1},
        {1, 1, 0, 0,    0, 0, 0, 0,     1, 0, 0, 0,     0, 0, 1, 0, 1},
        {1, 0, 0, 0,    0, 0, 0, 0,     1, 0, 0, 0,     0, 0, 0, 0, 1},
        {0, 0, 0, 0,    0, 1, 0, 0,     1, 0, 0, 0,     0, 0, 0, 0, 0},
        {1, 1, 0, 0,    0, 1, 0, 0,     1, 0, 0, 0,     0, 1, 0, 0, 1},
        {1, 0, 0, 0,    0, 1, 0, 0,     1, 0, 0, 0,     1, 0, 0, 0, 1},
        {1, 1, 1, 1,    1, 0, 0, 0,     0, 0, 0, 0,     0, 1, 0, 0, 1},
        {1, 1, 1, 1,    1, 1, 1, 1,     0, 1, 1, 1,     1, 1, 1, 1, 1}
    };
    real32 UpperLeftX = -30.0F;
    real32 UpperLeftY = 0.0F;
    real32 TileWidth = 60.0F;
    real32 TileHeight = 60.0F;
    ///> clear screem
    DrawRectangle(Buffer, 0.0F, 0.0F, (real32)Buffer->Width, (real32)Buffer->Height, 1.0F, 0.0F, 1.0F);
    ///> draw grid
    for(int Row = 0; Row < 9; Row++)
    {
        for(int Column = 0; Column < 17; Column++)
        {
            uint32_t TileID = TileMap[Row][Column];
            real32 Gray = 0.5F;
            if(TileID == 1)
            {
                Gray = 1.0F;
            }
            
            real32 MinX = UpperLeftX + (real32)(Column * TileWidth);
            real32 MinY = UpperLeftY + (real32)(Row * TileHeight);
            real32 MaxX = MinX + TileWidth;
            real32 MaxY = MinY + TileHeight;

            DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);        
        }
    }

    // GameState->PlayerX = 10.0F;
    // GameState->PlayerY = 10.0F;
    real32 PlayerR = 1.0F;
    real32 PlayerG = 1.0F;
    real32 PlayerB = 0.0F;
    real32 PlayerWidth = 0.75F * TileWidth;
    real32 PlayerHeight = TileHeight;
    real32 PlayerTop = GameState->PlayerY - PlayerHeight;
    real32 PlayerLeft = GameState->PlayerX - 0.5F * PlayerWidth;
    DrawRectangle(Buffer, PlayerLeft, PlayerTop,
                  PlayerLeft + PlayerWidth, PlayerTop + PlayerHeight,
                  PlayerR, PlayerG, PlayerB);        
}
extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameOutputSound(GameState, SoundBuffer, 400);
}

/*
internal void RenderWeirdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
    
    // Buffer->Pitch = Buffer->Width*Buffer->BytesPerPixel;

    uint8_t *Row = (uint8_t *)Buffer->Memory;
    for(int Y = 0; Y < Buffer->Height; ++Y){
        uint32_t *Pixel = (uint32_t *)Row;
        for(int X = 0; X < Buffer->Width; X++){
            uint8_t Blue = (uint8_t)(X + XOffset);
            uint8_t Green = (uint8_t)(Y + YOffset);
            
            *Pixel++ = ((Green << 16) | Blue);
        }
        Row += Buffer->Pitch;
    }
}
*/