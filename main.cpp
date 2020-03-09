#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>

#pragma comment (lib, "opengl32.lib")

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void Display();
HWND CreateOpenGLWindow(char* title, int x, int y, int width, int height,
                        BYTE type, DWORD flags);

HDC hDC;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
    // Use -municode arg for mingw_w64 to compile with unicode version of WinMain
    // Tutorial https://docs.microsoft.com/en-us/windows/win32/learnwin32/your-first-windows-program
    
    const LPCWSTR CLASS_NAME = L"Test win32api+OpenGL";
    MSG     msg; 
    //HDC     hDC;    // device context
    HGLRC   hRC;    // opengl context
    HWND    hwnd;   // window handle

    hwnd = CreateOpenGLWindow((char*)CLASS_NAME, 0, 0, 256, 256, PFD_TYPE_RGBA, 0);
    if (hwnd == NULL) {return 0;}

    hDC = GetDC(hwnd);
    hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);

    ShowWindow(hwnd, nCmdShow);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    wglMakeCurrent(hDC, NULL);
    ReleaseDC(hwnd, hDC);
    wglDeleteContext(hRC);
    DestroyWindow(hwnd);

    return 0;
}

void Display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex2i(0,  1);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex2i(-1, -1);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex2i(1, -1);
    glEnd();
    glFlush();
    SwapBuffers(hDC);
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static PAINTSTRUCT ps;

    switch (uMsg)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        case WM_PAINT:
        {
            Display();
            BeginPaint(hWnd, &ps);
            GLenum err;
            while ((err = glGetError()) != GL_NO_ERROR) {
                printf("OpenGL Error %x", err);
            }
            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_LBUTTONDOWN:
        {
            //MessageBox(hWnd, glGetString(GL_VERSION), L"OpenGL Version", 0);
            //MessageBox are corrupted, use printf to debug
            printf("Ver. rough: %s\nVer. casted: %s\n",
                    glGetString(GL_VERSION), (LPCWSTR)glGetString(GL_VERSION));
            return 0;
        }
        case WM_SIZE:
        {
            glViewport(0, 0, LOWORD(lParam), HIWORD(wParam));
            PostMessage(hWnd, WM_PAINT, 0, 0);
            return 0;
        }
        case WM_CHAR:
        {
            switch (wParam) {
                case 27:
                    PostQuitMessage(0);
                    return 0;
            }
            return 0;
        }
        
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

HWND CreateOpenGLWindow(char* title, int x, int y, int width, int height,
                        BYTE type, DWORD flags)
{
    int         pf;
    HDC         hDC;
    HWND        hWnd;
    WNDCLASS    wc;
    PIXELFORMATDESCRIPTOR pfd;
    static HINSTANCE hInstance = 0;

    if (!hInstance) {
        hInstance = GetModuleHandle(NULL);
        wc.style         = CS_OWNDC;
        wc.lpfnWndProc   = (WNDPROC)WindowProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = hInstance;
        wc.hIcon         = LoadIcon(NULL, IDI_WINLOGO);
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = NULL;
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = (LPCWSTR)title;

        if (!RegisterClass(&wc)) {
            MessageBox(NULL, L"RegisterClass() failed:  "
                L"Cannot register window class.", L"Error", MB_OK);
            return NULL;
        }
    }

    hWnd = CreateWindowEx(wc.style, (LPCWSTR)title, (LPCWSTR)title,
                        WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			            x, y, width, height, NULL, NULL, hInstance, NULL);

    // hWnd = CreateWindow((LPCWSTR)title, (LPCWSTR)title, WS_OVERLAPPEDWINDOW |
	// 	    WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
	// 		x, y, width, height, NULL, NULL, hInstance, NULL);

    if (hWnd == NULL) {
	    MessageBox(NULL, L"CreateWindow() failed:  Cannot create a window.",
		            L"Error", MB_OK);
	    return NULL;
    }

    hDC = GetDC(hWnd);
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize        = sizeof(pfd);
    pfd.nVersion     = 1;
    pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | flags;
    pfd.iPixelType   = type;
    pfd.cColorBits   = 32;

    pf = ChoosePixelFormat(hDC, &pfd);
    if (pf == 0) {
        MessageBox(NULL, L"ChoosePixelFormat() failed:  "
            L"Cannot find a suitable pixel format.", L"Error", MB_OK); 
        return NULL;
    }

    if (SetPixelFormat(hDC, pf, &pfd) == FALSE) {
        MessageBox(NULL, L"SetPixelFormat() failed:  "
            L"Cannot set format specified.", L"Error", MB_OK);
        return NULL;
    }

    DescribePixelFormat(hDC, pf, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

    ReleaseDC(hWnd, hDC);

    return hWnd;
}