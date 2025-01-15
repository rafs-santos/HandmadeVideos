#include <iostream>

#include <windows.h>

#define internal static
#define local_persist static 
#define global_variable static

global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContext;

internal void
Win32ResizeDIBSection(int Width, int Height){
    if(BitmapHandle){
        DeleteObject(BitmapHandle);
    }

    if(!BitmapDeviceContext){
        BitmapDeviceContext = CreateCompatibleDC(0);
    }
    // BITMAPINFO BitmapInfo;
    BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFO);
    BitmapInfo.bmiHeader.biWidth = Width;
    BitmapInfo.bmiHeader.biHeight = Height;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;
    // BitmapInfo.bmiHeader.biSizeImage = 0;
    // BitmapInfo.bmiHeader.biXPelsPerMeter = 0;
    // BitmapInfo.bmiHeader.biYPelsPerMeter = 0;
    // BitmapInfo.bmiHeader.biClrUsed = 0;
    // BitmapInfo.bmiHeader.biClrImportant = 0;

    BitmapHandle = CreateDIBSection(
        BitmapDeviceContext,
        &BitmapInfo,
        DIB_RGB_COLORS,
        &BitmapMemory,
        0,
        0
    );
}

internal void
Win32UpdateWindow(HDC DeviceContext, int X, int Y, int Width, int Height){
    StretchDIBits(
        DeviceContext,
        X, Y, Width, Height,
        X, Y, Width, Height,
        BitmapMemory,
        &BitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}


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
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            int Height = ClientRect.bottom - ClientRect.top;
            int Width = ClientRect.right - ClientRect.left;
            Win32ResizeDIBSection(Width, Height);
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
            Win32UpdateWindow(DeviceContext, X, Y, Width, Height);
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

    WNDCLASS WindowClass = {};
    // TODO(rafa): Check if CS_HEDRAW/CS_VREDRAW/OWNDC still matter
    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "HandMadeHeroWindowClass";

    // Register the window class.
    const wchar_t CLASS_NAME[]  = L"Sample Window Class";
    
    if(RegisterClass(&WindowClass)){
        HWND WindowHandle = CreateWindowEx(
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


        if(WindowHandle){
            Running = true;
            while(Running){
                MSG Message;
                BOOL MessageResult = GetMessage(&Message, NULL, 0, 0);
                if(MessageResult > 0) {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                } else {
                    break;
                }
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

