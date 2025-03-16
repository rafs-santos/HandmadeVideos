#include <stdint.h>

//input
/*
    TODO: THIS IS NOT A FINAL PLATFORM LAYER!!!
    - Saved game locations
    - Getting a handle to our own executable file
    - Asset loading path
    - Threading (launch a thread)
    - Raw Input (support for multiple keyboards)
    - Sleep/timeBegindPeriod
    - ClipCursor() (for multimonitor support)
    - Fullscreen support 
    - QueryCancelAutoplay
    - WM_ACTIVATEAPP (for when we are not the active application)
    - Blit speed improvements (BlitBlt)
    - Hardware acceleration (OpenGL or Direct3D or Both)
    - GetKeyboardLayout (for French keyboards)
*/

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
#include <math.h>

#include "handmade.cpp"

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <xinput.h>
#include <dsound.h>

#include "win32_handmade.h"

global_variable bool32 GlobalRunning;
global_variable bool32 GlobalPause;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64_t GlobalPerfCountFrequency;


#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)

typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub){
    return ERROR_DEVICE_NOT_CONNECTED;
}

typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub){
    return ERROR_DEVICE_NOT_CONNECTED;
}

global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal debug_read_file_result
DEBUGPlatformReadEntireFile(const char *Filename)
{
    debug_read_file_result Result = {};
    HANDLE FileHandle = CreateFileA(
        Filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        0,
        OPEN_EXISTING,
        0,
        0
    );

    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(FileHandle, &FileSize))
        {                  
            uint32_t FileSize32 = SafeTruncationUInt64(FileSize.QuadPart);
            DWORD BytesRead;
            Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if(Result.Contents)
            {
                if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
                   (FileSize32 == BytesRead))
                {
                    Result.ContentsSize = FileSize32;
                }
                else
                {
                    DEBUGPlatformFreeFileMemory(Result.Contents);
                    Result.Contents = NULL;
                }
            }
            else
            {
                //TODO: Logging
            }
        }    
        else
        {
            //TODO: Logging
        }
        CloseHandle(FileHandle);
    }
    else
    {
        //TODO: Logging
    }
    return Result;
}

internal void
DEBUGPlatformFreeFileMemory(void* Memory)
{
    if(Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

internal
bool32 DEBUGPlatformWriteEntireFile(const char *Filename, uint32_t MemorySize, void *Memory)
{
    bool32 Result = false;
    HANDLE FileHandle = CreateFileA(
        Filename,
        GENERIC_WRITE,
        0,
        0,
        CREATE_ALWAYS,
        0,
        0
    );

    if(FileHandle != INVALID_HANDLE_VALUE)
    {            
        DWORD BytesWritten;
        if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
        {
            Result = (BytesWritten == MemorySize);
        }
        else
        {
            //TODO: Logging
        }
        CloseHandle(FileHandle);
    }
    else
    {
        //TODO: Logging
    }
    return Result;
}


internal void
Win32LoadInput(void)
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }
    if(!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    if(XInputLibrary){
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}

internal void
Win32InitDSound(HWND Window, int32_t SamplesPerSecond, int32_t BufferSize)
{
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
    if(DSoundLibrary){
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
        
        LPDIRECTSOUND DirectSound;
        if(DirectSoundCreate && SUCCEEDED( DirectSoundCreate(0, &DirectSound, 0) ))
        {
            WAVEFORMATEX WaveFormat = {};
            
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample)/ 8;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;

            if( SUCCEEDED( DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY) )) {
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
                
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if(SUCCEEDED( DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0) )){
                    PrimaryBuffer->SetFormat(&WaveFormat);
                    OutputDebugStringA("Set_Format\n");
                }
            }
            else
            {

            }

            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;
            HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);
            if(SUCCEEDED(Error)){
                OutputDebugStringA("Set_Format 2\n");
            }


        }
        else
        {

        }
    }
}
internal 
win32_window_dimension Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension ret;

    RECT WindowRect;
    GetClientRect(Window, &WindowRect);
    ret.Height = WindowRect.bottom - WindowRect.top;
    ret.Width = WindowRect.right - WindowRect.left;

    return ret;

}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height){
    if(Buffer->Memory){
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }
    Buffer->Width = Width;
    Buffer->Height = Height;
    
    // BITMAPINFO BitmapInfo;
    // Buffer->Info.bmiHeader.biSize = sizeof(BITMAPINFO);
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Width;
    Buffer->Info.bmiHeader.biHeight = -Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    // BitmapInfo.bmiHeader.biSizeImage = 0;
    // BitmapInfo.bmiHeader.biXPelsPerMeter = 0;
    // BitmapInfo.bmiHeader.biYPelsPerMeter = 0;
    // BitmapInfo.bmiHeader.biClrUsed = 0;
    // BitmapInfo.bmiHeader.biClrImportant = 0;
    
    // NOTE: Chris Hecker (comment on the series about) StretchDIBits and BitBlt
    // No need to use DC (Device context)
    Buffer->BytesPerPixel = 4;
    int BitmapMemorySize = ((Width * Height) * Buffer->BytesPerPixel);
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Buffer->Width*Buffer->BytesPerPixel;
    // RenderWeirdGradient(0, 0);
    //TODO: clear window 
   
}

internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
                           HDC DeviceContext,
                           int WindowWidth, int WindowHeight
                           )
{
    StretchDIBits(
        DeviceContext,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer->Width, Buffer->Height,
        Buffer->Memory,
        &Buffer->Info,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

internal
LRESULT CALLBACK Win32MainWindowCallback(
    HWND Window,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
)
{
    LRESULT ret = 0;
    switch (Message)
    {
        case WM_CLOSE:
        {
            // OutputDebugStringA("WM_CLOSE\n");
            GlobalRunning = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_DESTROY:
        {
            // OutputDebugStringA("WM_DESTROY\n");
            GlobalRunning = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
                Assert(!"Keyboard input came in through a non-dispatch message!");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            win32_window_dimension WindowDimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, WindowDimension.Width, WindowDimension.Height);
            
            EndPaint(Window, &Paint);
        } break;

        default:
        {
            // OutputDebugStringA("default\n");
            ret = DefWindowProc(Window, Message, WParam, LParam);
            // ret = ;
        } break;
    }
    return ret;
}


internal void
Win32ClearBuffer(win32_sound_output *SoundOutput)
{
    void *Region1;
    DWORD Region1Size;
    void *Region2;
    DWORD Region2Size;

    //NOTE: Direct sound output test
    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
                                            &Region1, &Region1Size,
                                            &Region2, &Region2Size,
                                            0)))
    {
        int8_t *DestSample = (int8_t *)Region1;
        for(DWORD ByteIndex = 0; ByteIndex < Region1Size; ByteIndex++)
        {
            *DestSample++ = 0;
        }
        DestSample = (int8_t *)Region2;
        for(DWORD ByteIndex = 0; ByteIndex < Region2Size; ByteIndex++)
        {
            *DestSample++ = 0;
        }

        GlobalSecondaryBuffer->Unlock(Region1,Region1Size, Region2, Region2Size);
    }
}
internal void
Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD BytesToLock, DWORD BytesToWrite, game_sound_output_buffer *SourceBuffer)
{
    void *Region1;
    DWORD Region1Size;
    void *Region2;
    DWORD Region2Size;

    //NOTE: Direct sound output test
    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(BytesToLock, BytesToWrite,
                                &Region1, &Region1Size,
                                &Region2, &Region2Size,
                                0))
                )
    {
        DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
        int16_t *DestSample = (int16_t *)Region1;
        int16_t *SourceSample = SourceBuffer->Samples;
        for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            SoundOutput->RunningSampleIndex++;

        }
        DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
        DestSample = (int16_t *)Region2;
        for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex++)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            SoundOutput->RunningSampleIndex++;
        }
        GlobalSecondaryBuffer->Unlock(Region1,Region1Size, Region2, Region2Size);
        
    }
}

internal void
Win32ProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown)
{
    Assert(NewState->EndedDown != IsDown);
    NewState->EndedDown = IsDown;
    ++NewState->HalfTransistionCount;
}

internal void
Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state *OldState, DWORD ButtonBit,
                                game_button_state *NewState)
{
    if(OldState->EndedDown == NewState->EndedDown)
    {
        NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
        NewState->HalfTransistionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
    }
}

internal real32 Win32ProcessInputStickValue(SHORT Value, SHORT DeadZoneThresholdLeft, SHORT DeadZoneThresholdRight)
{
    real32 Ret = 0.0F;
    if(Value < -DeadZoneThresholdLeft)
    {
        Ret = (real32)((Value + DeadZoneThresholdLeft) / (32768.0F - DeadZoneThresholdLeft));
    }
    else if(Value > DeadZoneThresholdRight)
    {
        Ret = (real32)((Value - DeadZoneThresholdRight) / (32767.0F - DeadZoneThresholdRight));
    }
    return Ret;
}


internal void
Win32ProcessPendingMessages(game_controller_input *KeyboardController)
{
    MSG Message;
    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch (Message.message)
        {
            case WM_QUIT:
            {
                GlobalRunning = false;
            } break;

            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                
                uint32_t VKCode = (uint32_t)Message.wParam;
                bool32 WasDown  = ( (Message.lParam & (1 << 30)) != 0);
                bool32 IsDown  = ( (Message.lParam & (1 << 31)) == 0);
                if(WasDown != IsDown){
                    if(VKCode == 'W')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
                    }
                    else if(VKCode == 'A')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
                    }
                    else if(VKCode == 'S')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
                    }
                    else if(VKCode == 'D')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
                    }
                    else if(VKCode == 'Q')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
                    }
                    else if(VKCode == 'E')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
                    }
                    else if(VKCode == VK_UP)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
                    }
                    else if(VKCode == VK_LEFT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
                    }
                    else if(VKCode == VK_DOWN)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
                    }
                    else if(VKCode == VK_RIGHT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
                    }
                    else if(VKCode == VK_ESCAPE)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
                    }
                    else if(VKCode == VK_SPACE)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
                    }
#if HANDMADE_INTERNAL
                    else if(VKCode == 'P')
                    {
                        if(IsDown)
                        {
                            GlobalPause = !GlobalPause;
                        }
                        
                    }
#endif
                }
                bool32 AltKeyWasDown = ((Message.lParam & (1 << 29)));
                if(VKCode == VK_F4 && AltKeyWasDown){
                    GlobalRunning = false;
                }
            } break;
        default:
            {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            }
            break;
        }
    }
}

inline LARGE_INTEGER
Win32GetWallClock(void)
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);    
    return Result;
}

inline real32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    real32 Result =  ((real32)(End.QuadPart - Start.QuadPart) / 
                        (real32)GlobalPerfCountFrequency);
    return Result;
}

internal void
Win32DebugDrawVertical(win32_offscreen_buffer *Backbuffer, int X, int Top, int Bottom, uint32_t Color)
{
    if(Top <= 0)
    {
        Top = 0;
    }
    if(Bottom > Backbuffer->Height)
    {
        Bottom = Backbuffer->Height;
    }


    if((X >= 0) && (X < Backbuffer->Width))
    {
        uint8_t *Pixel = ((uint8_t *)Backbuffer->Memory + X * Backbuffer->BytesPerPixel +
                        Top * Backbuffer->Pitch);
        for(int Y = Top; Y < Bottom; Y++)
        {
            *(uint32_t *)Pixel = Color;
            Pixel += Backbuffer->Pitch;
        }
    }
}

inline void
Win32DrawSoundBufferMarker(win32_offscreen_buffer *Backbuffer,
                           win32_sound_output *SoundOutput,
                           real32 C, int PadX, int Top, int Bottom,
                           DWORD Value, uint32_t Color)
{
    real32 XReal32 = (C * (real32)Value);
    int X = PadX + (int)XReal32;

    Win32DebugDrawVertical(Backbuffer, X, Top, Bottom, Color);
}


internal void
Win32DebugSyncDisplay(win32_offscreen_buffer *Backbuffer, int MarkerCount, win32_debug_time_maker *Markers,
                      int CurrentMarkerIndex,win32_sound_output *SoundOutput,
                      real32 TargetSecondsPerFrame)
{
    int PadX = 16;
    int PadY = 16;

    int LineHeight = 64;

    real32 C = (real32)(Backbuffer->Width - 2*PadX) / (real32)SoundOutput->SecondaryBufferSize;
    for(int MarkerIndex = 0; MarkerIndex < MarkerCount; MarkerIndex++)
    {   
        win32_debug_time_maker *ThisMarker = &Markers[MarkerIndex];
        Assert(ThisMarker->OutputPlayCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputWriteCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputLocation < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputByteCount < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipPlayCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipWriteCursor < SoundOutput->SecondaryBufferSize);

        DWORD PlayColor = 0xFFFFFFFF;
        DWORD WriteColor = 0xFFFF0000;
        DWORD ExpectedFlipColor = 0xFFFFFF00;
        DWORD PlayWindowColor = 0xFFFF00FF;
        int Top = PadY;
        int Bottom = PadY + LineHeight;
        if(MarkerIndex == CurrentMarkerIndex)
        {
            Top += LineHeight + PadY;
            Bottom += LineHeight + PadY;
            int FirstTop = Top;
            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top,
                                       Bottom, ThisMarker->OutputPlayCursor, PlayColor);
            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top,
                                       Bottom, ThisMarker->OutputWriteCursor, WriteColor);

            Top += LineHeight + PadY;
            Bottom += LineHeight + PadY;

            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top,
                                       Bottom, ThisMarker->OutputLocation, PlayColor);
            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top,
                                       Bottom, ThisMarker->OutputLocation + ThisMarker->OutputByteCount, WriteColor);
            Top += LineHeight + PadY;
            Bottom += LineHeight + PadY;

            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, FirstTop,
                Bottom, ThisMarker->ExpectedFlipPlayCursor, ExpectedFlipColor);
        }
        
        
        Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top,
                                   Bottom, ThisMarker->FlipPlayCursor, PlayColor);
        Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top,
                                   Bottom, ThisMarker->FlipPlayCursor + 480*SoundOutput->BytesPerSample, PlayWindowColor);
        Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top,
                                   Bottom, ThisMarker->FlipWriteCursor, WriteColor);
    }   
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{   
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    
    //NOTE: Set the Windows scheduler granularity to 1ms
    // so that our Sleep() can be more granular
    UINT DesiredSchedulerMS = 1;
    bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

    Win32LoadInput();

    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";


#define MonitorRefreshHz 60
#define GameUpdateHz (MonitorRefreshHz / 2)
    
    real32 TargetSecondsPerFrame = 1.0F / GameUpdateHz;

    if(RegisterClass(&WindowClass)){
        HWND Window = CreateWindowEx(
            0,
            WindowClass.lpszClassName,
            "Handmade",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            Instance,
            0
        );


        if(Window)
        {   
            HDC DeviceContext = GetDC(Window);

            win32_sound_output SoundOutput = {};
            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.RunningSampleIndex = 0;
            
            SoundOutput.BytesPerSample = sizeof(int16_t)*2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
            
            SoundOutput.SafetyBytes = ((SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) /
                                             GameUpdateHz) / 3;
            SoundOutput.tSine = 0.0F;

            Win32InitDSound(Window,  SoundOutput.SamplesPerSecond,  SoundOutput.SecondaryBufferSize);
            Win32ClearBuffer(&SoundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
            
            GlobalRunning = true;
#if 0
            // NOTE: This test the PlayCursor/WriteCursor update frequency
            // It's 480 samples
            while(GlobalRunning)
            {
                DWORD PlayCursor;
                DWORD WriteCursor;
                GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
                char TextBuffer[256];
                sprintf_s(TextBuffer, sizeof(TextBuffer), "PC:%u WC:%u\n",
                          PlayCursor, WriteCursor);
                OutputDebugStringA(TextBuffer);
            }
#endif
            int16_t *Samples = (int16_t*)VirtualAlloc(0, SoundOutput.SecondaryBufferSize,
                                                      MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#if HANDMADE_INTERNAL
            LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
            LPVOID BaseAddress = 0;
#endif
            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes(64);
            GameMemory.TransientStorageSize = Gigabytes(1);
            
            uint64_t TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
            GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalSize,
                                                       MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            GameMemory.TransientStorage = ((uint8_t *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);
            
            if(Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
            {
                game_input Input[2] = {};
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];
    
                LARGE_INTEGER LastCounter = Win32GetWallClock();
                LARGE_INTEGER FlipWallClock = Win32GetWallClock();
                
                int DebugTimeMarkerIndex = 0;
                win32_debug_time_maker DebugTimeMarkers[GameUpdateHz / 2] = {};

                DWORD AudioLatencyBytes = 0;
                real32 AudioLatencySeconds = 0.0F;
                bool32 SoundIsValid = false;

                uint64_t LastCycleCount = __rdtsc();
                while(GlobalRunning)
                {
    
                    game_controller_input *OldKeyboardController = GetController(OldInput, 0);
                    game_controller_input *NewKeyboardController = GetController(NewInput, 0);
                    //TODO: Zeroing macro
                    // game_controller_input ZeroController = {};
                    *NewKeyboardController = {};
                    NewKeyboardController->IsConnected = true;
                    for(int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ButtonIndex++)
                    {
                        NewKeyboardController->Buttons[ButtonIndex].EndedDown = 
                            OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                    }

                    Win32ProcessPendingMessages(NewKeyboardController);
                    if(!GlobalPause)
                    {
                    //TODO: 
                    DWORD MaxControllerCount = XUSER_MAX_COUNT;
                    if(MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1))
                    {   
                        MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
                    }   
                    for (DWORD ControllerIndex=0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++ )
                    {
                        DWORD OurControllerIndex = ControllerIndex + 1;
                        game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
                        game_controller_input *NewController = GetController(NewInput, OurControllerIndex);
                        
                        XINPUT_STATE ControllerState;
                        // ZeroMemory( &ControllerState, sizeof(XINPUT_STATE) );
                        // Simply get the state of the controller from XInput.
                        if( XInputGetState( ControllerIndex, &ControllerState ) == ERROR_SUCCESS )
                        {
                            NewController->IsConnected = true;
                            // Controller is connected
                            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                            
                            NewController->StickAverageX = Win32ProcessInputStickValue(
                                Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,
                                XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
                            NewController->StickAverageY = Win32ProcessInputStickValue(
                                Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,
                                XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
                            
                            if((NewController->StickAverageX != 0.0F) ||
                               (NewController->StickAverageY != 0.0F))
                            {
                                NewController->IsAnalog = true; 
                            }

                            if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
                            {
                                NewController->StickAverageY = 1.0F;
                                NewController->IsAnalog = false;
                            }
                            if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
                            {
                                NewController->StickAverageY = -1.0F;
                                NewController->IsAnalog = false;
                            }
                            if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
                            {
                                NewController->StickAverageX = 1.0F;
                                NewController->IsAnalog = false;
                            }
                            if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
                            {
                                NewController->StickAverageX = -1.0F;
                                NewController->IsAnalog = false;
                            }

                            real32 Threshold = 0.5F;
                            Win32ProcessXInputDigitalButton(
                                (NewController->StickAverageX < -Threshold) ? 1 : 0,
                                &OldController->MoveLeft, 1,
                                &NewController->MoveLeft);

                            Win32ProcessXInputDigitalButton(
                                (NewController->StickAverageX > Threshold) ? 1 : 0,
                                &OldController->MoveRight, 1,
                                &NewController->MoveRight);

                            Win32ProcessXInputDigitalButton(
                                (NewController->StickAverageY < -Threshold) ? 1 : 0,
                                &OldController->MoveDown, 1,
                                &NewController->MoveDown);

                            Win32ProcessXInputDigitalButton(
                                (NewController->StickAverageY > Threshold) ? 1 : 0,
                                &OldController->MoveUp, 1,
                                &NewController->MoveUp);

                            Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                            &OldController->ActionDown, XINPUT_GAMEPAD_A,
                                                            &NewController->ActionDown);

                            Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                            &OldController->ActionRight, XINPUT_GAMEPAD_B,
                                                            &NewController->ActionRight);

                            Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                            &OldController->ActionLeft, XINPUT_GAMEPAD_X,
                                                            &NewController->ActionLeft);

                            Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                            &OldController->ActionUp, XINPUT_GAMEPAD_Y,
                                                            &NewController->ActionUp);

                            Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                            &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
                                                            &NewController->LeftShoulder);

                            Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                            &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                                            &NewController->RightShoulder);

                            Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                            &OldController->Start, XINPUT_GAMEPAD_START,
                                                            &NewController->Start);

                            Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                            &OldController->Back, XINPUT_GAMEPAD_BACK,
                                                            &NewController->Back);
                            
                        }
                        else
                        {
                            NewController->IsConnected = false;
                            // Controller is not connected
                        }
                    }
                    
                    game_offscreen_buffer Buffer = {};
                    Buffer.Memory = GlobalBackBuffer.Memory;
                    Buffer.Width = GlobalBackBuffer.Width;
                    Buffer.Height = GlobalBackBuffer.Height;
                    Buffer.Pitch = GlobalBackBuffer.Pitch;
                    GameUpdateAndRender(&GameMemory, NewInput, &Buffer);

                    LARGE_INTEGER AudioWallClock = Win32GetWallClock();
                    real32 FromBeginToAudioSeconds =  (1000.0F * Win32GetSecondsElapsed(FlipWallClock, AudioWallClock));

                    DWORD PlayCursor = 0;
                    DWORD WriteCursor = 0;
                    if(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
                    {
                        /* NOTE:
                        Discribe how sound output works 

                        We define a safety value that is the number of samples we think our game update loop may 
                        vary by (let's say up to 2ms)

                        When we wake up to write audio, we will look and see what the play cursor position is and
                        we will forecast ahead where we think the play cursor will be on the next frame boundary.

                        We will then look to see if the write cursor is before that by at least our safety value.
                        If it is, the target fill position is that frame boundary plus one frame. This gives us perfect
                        audio sync in the case of a card that has low enough latency.
                        

                        If the write cursor is _after_ that safety margin, then we assume we can never sync
                        the audio perfectly, so we will write one frame's worth of audio plus the safety margin's 
                        of guard samples.
                        */
                        
                        if(!SoundIsValid)
                        {
                            SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
                            SoundIsValid = true;
                        }

                        DWORD BytesToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;

                        DWORD ExpectedSoundBytesPerFrame =
                            (SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) / GameUpdateHz;
                        real32 SencondsLeftUntilFlip = TargetSecondsPerFrame - FromBeginToAudioSeconds;
                        DWORD ExpectedBytesUntilFlip = (DWORD)( (SencondsLeftUntilFlip/TargetSecondsPerFrame) * (real32)ExpectedSoundBytesPerFrame);
                        DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedSoundBytesPerFrame;
                        DWORD SafeWriteCursor = WriteCursor;
                        if(SafeWriteCursor < PlayCursor)
                        {
                            SafeWriteCursor += SoundOutput.SecondaryBufferSize;
                        }
                        Assert(SafeWriteCursor >= PlayCursor);
                        SafeWriteCursor += SoundOutput.SafetyBytes;

                        bool32 AudioCardIsLowLatency = SafeWriteCursor < ExpectedFrameBoundaryByte;
                        
                        DWORD TargetCursor = 0;
                        if(AudioCardIsLowLatency)
                        {
                            TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
                        }
                        else
                        {
                            TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes);
                            
                        }
                        TargetCursor = (TargetCursor  % SoundOutput.SecondaryBufferSize);
                        
                        DWORD BytesToWrite = 0;
                        if(BytesToLock > TargetCursor)
                        {
                            BytesToWrite = (SoundOutput.SecondaryBufferSize - BytesToLock);
                            BytesToWrite += TargetCursor;
                        } else
                        {
                            BytesToWrite = TargetCursor - BytesToLock;
                        }
                        game_sound_output_buffer SoundBuffer = {};
                        SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                        SoundBuffer.SampleCount =  BytesToWrite / SoundOutput.BytesPerSample;
                        SoundBuffer.Samples = Samples;
                        GameGetSoundSamples(&GameMemory, &SoundBuffer);
#if HANDMADE_INTERNAL
                        win32_debug_time_maker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
                        Marker->OutputPlayCursor = PlayCursor;
                        Marker->OutputWriteCursor = WriteCursor;
                        Marker->OutputLocation = BytesToLock;
                        Marker->OutputByteCount = BytesToWrite;
                        Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;
                        DWORD UnwrappedWriteCursor = WriteCursor;
                        if(UnwrappedWriteCursor < PlayCursor)
                        {
                            UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
                        }
                        
                        AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;
                        AudioLatencySeconds = (((real32)AudioLatencyBytes / (real32)SoundOutput.BytesPerSample) /
                                                     (real32)SoundOutput.SamplesPerSecond);
                        
                        char TextBuffer[256];
                        sprintf_s(TextBuffer, sizeof(TextBuffer), "BTL:%u TC:%u BTW:%u - PC:%u WC:%u DELTA:%u (%fs)\n",
                                  BytesToLock, TargetCursor, BytesToWrite,
                                  PlayCursor, WriteCursor, AudioLatencyBytes, AudioLatencySeconds);
                        OutputDebugStringA(TextBuffer);
#endif
                        Win32FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite, &SoundBuffer);
                    }
                    else
                    {
                        SoundIsValid = false;
                    }
                    
                    LARGE_INTEGER WorkCounter = Win32GetWallClock();
                    real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

                    real32 SecondsElapsedForFrame = WorkSecondsElapsed;
                    if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                    {
                        if(SleepIsGranular)
                        {
                            DWORD SleepMS = (DWORD)(1000.0F * (TargetSecondsPerFrame - SecondsElapsedForFrame));
                            if(SleepMS > 0)
                            {
                                Sleep(SleepMS);
                            }
                        }

                        real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                        
                        if(TestSecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            //TODO: LOG HERE MISSED SLEEP 
                        }

                        while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {   
                            SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                        }
                    }
                    else
                    {
                        //TODO: Missed frame rate
                        //TODO: Logging
                    }

                    LARGE_INTEGER EndCounter = Win32GetWallClock();
                    real32 MSPerFrame =  (1000.0F * Win32GetSecondsElapsed(LastCounter, EndCounter));
                    LastCounter = EndCounter;

                    win32_window_dimension WindowDimension = Win32GetWindowDimension(Window);
#if HANDMADE_INTERNAL
                    Win32DebugSyncDisplay(&GlobalBackBuffer, ArrayCount(DebugTimeMarkers), DebugTimeMarkers,
                                          DebugTimeMarkerIndex -1, &SoundOutput, TargetSecondsPerFrame);
#endif                    
                    Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, WindowDimension.Width,
                                               WindowDimension.Height);
                    
                    FlipWallClock = Win32GetWallClock();
#if HANDMADE_INTERNAL
                    {
                        DWORD PlayCursor2 = 0;
                        DWORD WriteCursor2 = 0;
                        if(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor2, &WriteCursor2) == DS_OK)
                        {
                            Assert(DebugTimeMarkerIndex < ArrayCount(DebugTimeMarkers));
                            win32_debug_time_maker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
                            Marker->FlipPlayCursor = PlayCursor2;
                            Marker->FlipWriteCursor = WriteCursor2;
                        }
                    }
#endif


                    game_input *Temp = NewInput;
                    NewInput = OldInput;
                    OldInput = Temp;

                    uint64_t EndCycleCount = __rdtsc();
                    uint64_t CyclesElapsed = EndCycleCount - LastCycleCount;
                    LastCycleCount = EndCycleCount;

                    
                    float FPS =  0.0F;
                    float MCPF = ((float)CyclesElapsed / (1000.0F * 1000.0F));

                    char FPSBuffer[256];
                    sprintf_s(FPSBuffer, sizeof(FPSBuffer), "%.02fms/f,  %.02ff/s,  %.02fmc/f\n", MSPerFrame, FPS, MCPF);
                    OutputDebugStringA(FPSBuffer);
#if HANDMADE_INTERNAL
                    DebugTimeMarkerIndex++;
                    if(DebugTimeMarkerIndex == ArrayCount(DebugTimeMarkers))
                    {
                        DebugTimeMarkerIndex = 0;
                    }
#endif
                }
                }
            }
            else
            {

            }
        } else 
        {
            //TODO(rafa): Loggin messages
        }

    } 
    else 
    {
        //TODO(rafa): Loggin messages
    }



    return 0;
}

