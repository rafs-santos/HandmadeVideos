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

#include <math.h>
#include <stdint.h>

#define internal static
#define local_persist static 
#define global_variable static

#ifndef M_PI
#define PI32 3.14159265358979323846264338327950288419716939937510F
#else
#define PI32 M_PI
#endif

typedef int32_t bool32;
typedef float real32;
typedef double real64;


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

/*
    NOTE: Services that the game provides to the platform layer.
*/

/*
    NOTE: Services that the platform layer provides to the game.
*/
#if HANDMADE_INTERNAL
struct debug_read_file_result {
    uint32_t ContentsSize;
    void *Contents;
};

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void* Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(const char *Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(const char *Filename, uint32_t MemorySize, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);


#endif

// timing, controller/keyboard input, bitmap buffer to use, sound buffer to use
// ***************************** Stop Platform-indenpedent Game Memory
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

struct game_button_state
{
    int HalfTransistionCount;
    bool32 EndedDown;
};

struct game_controller_input
{
    bool32 IsConnected;
    bool32 IsAnalog;
    real32 StickAverageX;
    real32 StickAverageY;

    union 
    {
        game_button_state Buttons[12];
        struct
        {
            game_button_state MoveUp;
            game_button_state MoveDown;
            game_button_state MoveLeft;
            game_button_state MoveRight;

            game_button_state ActionUp;
            game_button_state ActionDown;
            game_button_state ActionLeft;
            game_button_state ActionRight;

            game_button_state LeftShoulder;
            game_button_state RightShoulder;

            game_button_state Back;
            game_button_state Start;


            //
            game_button_state Terminator;
        };
        /* data */
    }; 
};

struct game_input
{
    game_controller_input Controllers[5];
};

inline game_controller_input *GetController(game_input *Input, int unsigned ControllerIndex)
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    game_controller_input *Result = &Input->Controllers[ControllerIndex];

    return Result;
}

struct game_memory
{
    bool32 IsInitialize;
    uint64_t PermanentStorageSize;
    void *PermanentStorage;

    uint64_t TransientStorageSize;
    void *TransientStorage;

    debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
    debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
    debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub)
{
}

#define GAME_GET_SOUND_SAMPLES(name) void name(game_memory *Memory, game_sound_output_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);
GAME_GET_SOUND_SAMPLES(GameGetSoundSamplesStub)
{
}
// void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer);
// void GameGetSoundSamples(game_memory *Memory, game_sound_output_buffer *SoundBuffer);
struct game_state
{
    int ToneHz;
    int GreenOffset;
    int BlueOffset;

    real32 tSine;
};

/*
    NOTE: Services that the platform layer provides to the platform layer.
*/


#endif