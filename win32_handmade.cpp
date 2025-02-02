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
#include <math.h>

#include "handmade.cpp"

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <xinput.h>
#include <dsound.h>

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct win32_window_dimension
{
    int Width;
    int Height;
};

global_variable bool32 GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

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
            BufferDescription.dwFlags = 0;
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
    Buffer->Info.bmiHeader.biSize = sizeof(BITMAPINFO);
    Buffer->Info.bmiHeader.biWidth = Width;
    Buffer->Info.bmiHeader.biHeight = Height;
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
    case WM_SIZE:
        {
            // win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            // Win32ResizeDIBSection(&GlobalBackBuffer, Dimension.Width, Dimension.Height);
        }
        break;
    case WM_CLOSE:
        {
            // OutputDebugStringA("WM_CLOSE\n");
            GlobalRunning = false;
        }
        break;
    case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        }
        break;
    case WM_DESTROY:
        {
            // OutputDebugStringA("WM_DESTROY\n");
            GlobalRunning = false;
        }
        break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
        {
            uint32_t VKCode = WParam;
            bool32 WasDown  = ( (LParam & (1 << 30)) != 0);
            bool32 IsDown  = ( (LParam & (1 << 31)) == 0);
            if(WasDown != IsDown){
                if(VKCode == 'W'){

                }
                else if(VKCode == 'A'){

                }
                else if(VKCode == 'S'){

                }
                else if(VKCode == 'D'){

                }
                else if(VKCode == 'Q'){

                }
                else if(VKCode == 'E'){

                }
                else if(VKCode == VK_UP){

                }
                else if(VKCode == VK_LEFT){

                }
                else if(VKCode == VK_DOWN){

                }
                else if(VKCode == VK_RIGHT){

                }
                else if(VKCode == VK_ESCAPE){
                    if(IsDown){
                        OutputDebugStringA("IsDown\n");
                    }
                    if(WasDown){
                        OutputDebugStringA("WasDown\n");
                    }
                }
            }
            bool32 AltKeyWasDown = ((LParam & (1 << 29)));
            if(VKCode == VK_F4 && AltKeyWasDown){
                GlobalRunning = false;
            }

        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            win32_window_dimension WindowDimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, WindowDimension.Width, WindowDimension.Height);
            
            EndPaint(Window, &Paint);
        }
        break;
    default:
        {
            // OutputDebugStringA("default\n");
            ret = DefWindowProc(Window, Message, WParam, LParam);
            // ret = ;
        }
        break;
    }
    return ret;
}

struct win32_sound_output
{
    int SamplesPerSecond;
    int ToneHz;
    int16_t ToneVolume;
    uint32_t RunningSampleIndex;

    int WavePeriod;
    int HalfWavePeriod;
    int BytesPerSample;
    int SecondaryBufferSize;
    float tSine;
    int LatencySampleCount;
};
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


int WINAPI WinMain(HINSTANCE Instance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{   
    Win32LoadInput();

    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    // TODO(rafa): Check if CS_HEDRAW/CS_VREDRAW/OWNDC still matter
    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    int64_t PerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    // Register the window class.
    const wchar_t CLASS_NAME[]  = L"Sample Window Class";
    
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

            int XOffset = 0;
            int YOffset = 0;

            win32_sound_output SoundOutput = {};
            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.ToneHz = 256;
            SoundOutput.ToneVolume = 2000;
            SoundOutput.RunningSampleIndex = 0;
            
            SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;
            SoundOutput.HalfWavePeriod = SoundOutput.WavePeriod / 2;
            SoundOutput.BytesPerSample = sizeof(int16_t)*2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
            SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 16;
            SoundOutput.tSine = 0.0F;

            Win32InitDSound(Window,  SoundOutput.SamplesPerSecond,  SoundOutput.SecondaryBufferSize);
            Win32ClearBuffer(&SoundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
            
            GlobalRunning = true;

            int16_t *Samples = (int16_t*)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            LARGE_INTEGER LastCounter;
            QueryPerformanceCounter(&LastCounter);
            uint64_t LastCycleCount = __rdtsc();

            while(GlobalRunning)
            {
                MSG Message;
                while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if(Message.message == WM_QUIT)
                    {
                        GlobalRunning = false;
                    }
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);

                    //TODO: 
                    for (DWORD ControllerIndex=0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++ )
                    {
                        XINPUT_STATE ControllerState;
                        ZeroMemory( &ControllerState, sizeof(XINPUT_STATE) );

                        // Simply get the state of the controller from XInput.
                        if( XInputGetState( ControllerIndex, &ControllerState ) == ERROR_SUCCESS )
                        {
                            // Controller is connected
                            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                            bool32 Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                            bool32 Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                            bool32 Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                            bool32 Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                            
                            bool32 Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                            bool32 Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                            bool32 LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                            bool32 RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);

                            bool32 AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                            bool32 BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                            bool32 XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                            bool32 YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);
                            
                            int16_t StickX = Pad->sThumbLX;
                            int16_t StickY = Pad->sThumbLY;

                            XOffset += StickX >> 12; 
                            YOffset += StickY >> 12;

                            if(AButton)
                            {
                                SoundOutput.ToneHz = 512;
                                SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
                            }
                        }
                        else
                        {
                            // Controller is not connected
                        }
                    }
                }

                    
                DWORD PlayCursor;
                DWORD WriteCursor;
                DWORD BytesToLock;
                DWORD TargetCursor;
                DWORD BytesToWrite;
                bool32 SoundIsValid = false;
                // STOP HANDMADE DAY 9 IN 1 hour and 47Minutes;
                if( SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)) )
                {
                    BytesToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
                    TargetCursor = ((PlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize);

                    if(BytesToLock > TargetCursor)
                    {
                        BytesToWrite = (SoundOutput.SecondaryBufferSize - BytesToLock);
                        BytesToWrite += TargetCursor;
                    } else
                    {
                        BytesToWrite = TargetCursor - BytesToLock;
                    }
                    SoundIsValid = true;
                }


               
                game_sound_output_buffer SoundBuffer = {};
                SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                SoundBuffer.SampleCount =  BytesToWrite / SoundOutput.BytesPerSample;
                SoundBuffer.Samples = Samples;

                game_offscreen_buffer Buffer = {};
                Buffer.Memory = GlobalBackBuffer.Memory;
                Buffer.Width = GlobalBackBuffer.Width;
                Buffer.Height = GlobalBackBuffer.Height;
                Buffer.Pitch = GlobalBackBuffer.Pitch;

                GameUpdateAndRender(&Buffer, XOffset, YOffset, &SoundBuffer, SoundOutput.ToneHz);
                

                // STOP HANDMADE DAY 9 IN 1 hour and 47Minutes;
                if(SoundIsValid)
                {
                    Win32FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite, &SoundBuffer);
                }

                win32_window_dimension WindowDimension = Win32GetWindowDimension(Window);
                Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, WindowDimension.Width, WindowDimension.Height);
                // ReleaseDC(Window, DeviceContext);

                ++XOffset;
                
                uint64_t EndCycleCount = __rdtsc();

                LARGE_INTEGER EndCounter;
                QueryPerformanceCounter(&EndCounter);

                //TODO: Display the value here
                uint64_t CyclesElapsed = EndCycleCount - LastCycleCount;
                int64_t CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
                float MSPerFrame =  ((1000.0F * (float)CounterElapsed) / (float)PerfCountFrequency);
                float FPS =  (float)PerfCountFrequency / (float)CounterElapsed;
                float MCPF = ((float)CyclesElapsed / (1000.0F * 1000.0F));
#if 0
                char Buffer[256];
                sprintf(Buffer, "%.02fms/f,  %.02ff/s,  %.02fmc/f\n", MSPerFrame, FPS, MCPF);
                OutputDebugStringA(Buffer);
#endif
                LastCounter = EndCounter;
                LastCycleCount = EndCycleCount;
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

