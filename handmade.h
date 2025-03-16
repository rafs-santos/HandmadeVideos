#ifndef HANDMADE_H
#define HANDMADE_H

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
internal debug_read_file_result DEBUGPlatformReadEntireFile(const char *Filename);
internal void DEBUGPlatformFreeFileMemory(void* Memory);
internal bool32 DEBUGPlatformWriteEntireFile(const char *Filename, uint32_t MemorySize, void *Memory);
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
};

internal void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer);
internal void GameGetSoundSamples(game_memory *Memory, game_sound_output_buffer *SoundBuffer);
struct game_state
{
    int ToneHz;
    int GreenOffset;
    int BlueOffset;
};

/*
    NOTE: Services that the platform layer provides to the platform layer.
*/


#endif