#ifndef HANDMADE_H
#define HANDMADE_H
/*
    NOTE:
    HANDMADE_INTERNAL:
        0 - Build for public release
        1 - Build for developer only
    
    HANDMADE_SLOW:
        0 - Not slow code allowed!
        1 - Slow code welcome.
*/
#include "handmade_platform.h"

#define internal static
#define local_persist static 
#define global_variable static

#ifndef M_PI
#define PI32 3.14159265358979323846264338327950288419716939937510F
#else
#define PI32 M_PI
#endif

#if HANDMADE_SLOW
#define Assert(Expression) \
    if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array)/ sizeof((Array)[0]))

inline uint32_t
SafeTruncationUInt64(uint64_t Value)
{
    Assert(Value <= 0xFFFFFFFF);
    uint32_t Result = (uint32_t)Value;   
    return Result;
}

inline game_controller_input *GetController(game_input *Input, int unsigned ControllerIndex)
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    
    game_controller_input *Result = &Input->Controllers[ControllerIndex];
    return(Result);
}

// void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer);
// void GameGetSoundSamples(game_memory *Memory, game_sound_output_buffer *SoundBuffer);
struct canonical_position
{
#if 1
    int32_t TileMapX;
    int32_t TileMapY;

    int32_t TileX;
    int32_t TileY;
#else
    int32_t TileX;
    int32_t TileY;
#endif
    // NOTE: This is tile-relative X and Y
    // TODO: These are still in pixels... :/
    real32 TileRelX;
    real32 TileRelY;
};

struct tile_map
{
    uint32_t *Tiles;
};

struct world
{
    real32 TileSideInMeters;
    int32_t TileSideInPixels;
    real32 MetersToPixels;
    
    int32_t CountX;
    int32_t CountY;
    
    real32 UpperLeftX;
    real32 UpperLeftY;

    // TODO(casey): Beginner's sparseness
    int32_t TileMapCountX;
    int32_t TileMapCountY;
    
    tile_map *TileMaps;
};


struct game_state
{
    canonical_position PlayerP;
};

/*
    NOTE: Services that the platform layer provides to the platform layer.
*/


#endif
