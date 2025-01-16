#include <iostream>
#include <stdint.h>

//input


#include <windows.h>
#include <xinput.h>
#define internal static
#define local_persist static 
#define global_variable static


struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)

typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub){
    return 0;
}

typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub){
    return 0;
}

global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_


internal void
Win32LoadInput(void)
{
    HMODULE XInputLibrary = LoadLibrary("xinput1_3.dll");
    if(XInputLibrary){
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}


global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;


struct win32_window_dimension
{
    int Width;
    int Height;
};
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
RenderWeirdGradient(win32_offscreen_buffer Buffer, int XOffset, int YOffset){
    
    // Buffer->Pitch = Buffer->Width*Buffer->BytesPerPixel;

    uint8_t *Row = (uint8_t *)Buffer.Memory;
    for(int Y = 0; Y < Buffer.Height; ++Y){
        uint32_t *Pixel = (uint32_t *)Row;
        for(int X = 0; X < Buffer.Width; X++){
            /*
                Pixel in memory BB GG RR XX

            */
            uint8_t Blue = (X + XOffset);
            uint8_t Green = (Y + YOffset);
            
            *Pixel++ = ((Green << 8) | Blue);
        }
        Row += Buffer.Pitch;
    }
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
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Buffer->Width*Buffer->BytesPerPixel;
    // RenderWeirdGradient(0, 0);
    //TODO: clear window 
   
}

internal void
Win32DisplayBufferInWindow(HDC DeviceContext,
                           win32_offscreen_buffer *Buffer,
                           int WindowWidth, int WindowHeight,
                           int X, int Y, int Width, int Height)
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
            Running = false;
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
            Running = false;
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;

            win32_window_dimension WindowDimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(DeviceContext, &GlobalBackBuffer, WindowDimension.Width, WindowDimension.Height, X, Y, Width, Height);
            
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


int WINAPI WinMain(HINSTANCE Instance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{   
    Win32LoadInput();

    WNDCLASS WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    // TODO(rafa): Check if CS_HEDRAW/CS_VREDRAW/OWNDC still matter
    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

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
            Running = true;
            while(Running)
            {
      
                MSG Message;
                while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if(Message.message == WM_QUIT)
                    {
                        Running = false;
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

                            bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                            bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                            bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                            bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                            
                            bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                            bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                            bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                            bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);

                            bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                            bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                            bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                            bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);
                            
                            int16_t StickX = Pad->sThumbLX;
                            int16_t StickY = Pad->sThumbLY;
                        }
                        else
                        {
                            // Controller is not connected
                        }
                    }
                }

                RenderWeirdGradient(GlobalBackBuffer, XOffset, YOffset);

                win32_window_dimension WindowDimension = Win32GetWindowDimension(Window);
                Win32DisplayBufferInWindow(DeviceContext, &GlobalBackBuffer, WindowDimension.Width, WindowDimension.Height, 0, 0, 0, 0);
                ReleaseDC(Window, DeviceContext);

                ++XOffset;
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

