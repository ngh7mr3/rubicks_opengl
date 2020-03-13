
/* An example of processing mouse events in an OpenGL program using
   the Win32 API. */


#include <windows.h>			/* must include this before GL/gl.h */
#include <GL/gl.h>			/* OpenGL header file */
#include <GL/glu.h>			/* OpenGL utilities header file */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define unpack(x) x[0],x[1],x[2]
#define clamp(x) x = x > 360.0f ? x-360.0f : x < -360.0f ? x+=360.0f : x

enum { 
    PAN = 1,				/* pan state bit */
    ROTATE,				/* rotate state bits */
    ZOOM				/* zoom state bit */
};

struct GLCubeSide {
	GLint *tl, *tr, *br, *bl;
	GLfloat r, g, b;
};

// dots front, clockwise
// dots back, clockwise
GLint CUBE[8][3] = {{1,-1,1},{1,1,1},{1,1,-1},{1,-1,-1},
			{-1,-1,1},{-1,1,1},{-1,1,-1},{-1,-1,-1}};
GLboolean CUBE_INITIALIZED = GL_FALSE;
struct GLCubeSide Sides[6];

HDC hDC;					/* device context */
HPALETTE hPalette = 0;		/* custom palette (if needed) */
GLfloat trans[3];			/* current translation */
GLfloat rot[2];				/* current rotation */

GLfloat glRandf() {
	return (GLfloat)rand()/(GLfloat)RAND_MAX;
}

void drawCubeSide(struct GLCubeSide* side) {
	glBegin(GL_TRIANGLE_STRIP);
		glColor3f(fabsf(side->r), fabsf(side->g), fabsf(side->b));
		glVertex3i(unpack(side->tl));
		glVertex3i(unpack(side->tr));
		glVertex3i(unpack(side->bl));
		glVertex3i(unpack(side->br));
	glEnd();
}

void processCubeColors() {
	for (int i=0; i<6; i++) {
		Sides[i].r += 0.001f; Sides[i].r -= Sides[i].r>1 ? 2.0f: 0.0f;
		Sides[i].g += 0.001f; Sides[i].g -= Sides[i].g>1 ? 2.0f: 0.0f;
		Sides[i].b += 0.001f; Sides[i].b -= Sides[i].b>1 ? 2.0f: 0.0f;
	}
}

void initCube() {
	// Front, Back, Left, Right, Top, Bottom;
	Sides[0] = (struct GLCubeSide){CUBE[0], CUBE[1], CUBE[2], CUBE[3], glRandf(), glRandf(), glRandf()};
	Sides[1] = (struct GLCubeSide){CUBE[4], CUBE[5], CUBE[6], CUBE[7], glRandf(), glRandf(), glRandf()};
	Sides[2] = (struct GLCubeSide){CUBE[0], CUBE[4], CUBE[7], CUBE[3], glRandf(), glRandf(), glRandf()};
	Sides[3] = (struct GLCubeSide){CUBE[1], CUBE[5], CUBE[6], CUBE[2], glRandf(), glRandf(), glRandf()};
	Sides[4] = (struct GLCubeSide){CUBE[0], CUBE[4], CUBE[5], CUBE[1], glRandf(), glRandf(), glRandf()};
	Sides[5] = (struct GLCubeSide){CUBE[3], CUBE[7], CUBE[6], CUBE[2], glRandf(), glRandf(), glRandf()};
	CUBE_INITIALIZED = GL_TRUE;
}

static void update(int state, int ox, int nx, int oy, int ny)
{
	int dx = ox - nx;
	int dy = ny - oy;

    switch(state) {
		case PAN:
			trans[0] -= dx / 100.0f;
			trans[1] -= dy / 100.0f;
			break;
		case ROTATE:
			rot[0] += (dy * 180.0f) / 500.0f;
			rot[1] -= (dx * 180.0f) / 500.0f;
			clamp(rot[0]);
			clamp(rot[1]);
			break;
		case ZOOM:
			trans[2] -= (dx+dy) / 100.0f;
			break;
    }
}


void
init()
{
    glEnable(GL_DEPTH_TEST);
}

void
reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (float)width/height, 0.001, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -3.0f);
}

void
display()
{
    /* rotate cube*/
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();
    glTranslatef(trans[0], trans[1], trans[2]);
    glRotatef(rot[0], 1.0f, 0.0f, 0.0f);
    glRotatef(rot[1], 0.0f, 1.0f, 0.0f);

	/* init cube if it isn't*/
	if (!CUBE_INITIALIZED) {initCube();}

	/* process new colors, mod by 1 */
	processCubeColors();

	/* draw updated sides of cube */
	for (int i=0; i<6; i++) {drawCubeSide(&Sides[i]);}

    glPopMatrix();
    glFlush();
    SwapBuffers(hDC);			/* nop if singlebuffered */
}


LONG WINAPI
WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{ 
    static PAINTSTRUCT ps;
    static GLboolean left  = GL_FALSE;	/* left button currently down? */
    static GLboolean right = GL_FALSE;	/* right button currently down? */
    static GLuint    state   = 0;	/* mouse state flag */
    static int omx, omy, mx, my;

    switch(uMsg) {
    case WM_PAINT:
	display();
	BeginPaint(hWnd, &ps);
	EndPaint(hWnd, &ps);
	return 0;

    case WM_SIZE:
	reshape(LOWORD(lParam), HIWORD(lParam));
	PostMessage(hWnd, WM_PAINT, 0, 0);
	return 0;

    case WM_CHAR:
	switch (wParam) {
	case 27:			/* ESC key */
	    PostQuitMessage(0);
	    break;
	}
	return 0;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
	/* if we don't set the capture we won't get mouse move
           messages when the mouse moves outside the window. */
	SetCapture(hWnd);
	mx = LOWORD(lParam);
	my = HIWORD(lParam);
	if (uMsg == WM_LBUTTONDOWN)
	    state |= PAN;
	if (uMsg == WM_RBUTTONDOWN)
	    state |= ROTATE;
	return 0;

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
	/* remember to release the capture when we are finished. */
	ReleaseCapture();
	state = 0;
	return 0;

    case WM_MOUSEMOVE:
	if (state) {
	    omx = mx;
	    omy = my;
	    mx = LOWORD(lParam);
	    my = HIWORD(lParam);
	    /* Win32 is pretty braindead about the x, y position that
	       it returns when the mouse is off the left or top edge
	       of the window (due to them being unsigned). therefore,
	       roll the Win32's 0..2^16 pointer co-ord range to the
	       more amenable (and useful) 0..+/-2^15. */
	    if(mx & 1 << 15) mx -= (1 << 16);
	    if(my & 1 << 15) my -= (1 << 16);
	    update(state, omx, mx, omy, my);
	    PostMessage(hWnd, WM_PAINT, 0, 0);
	}
	return 0;

    case WM_PALETTECHANGED:
	if (hWnd == (HWND)wParam)
	    break;
	/* fall through to WM_QUERYNEWPALETTE */

    case WM_QUERYNEWPALETTE:
	if (hPalette) {
	    UnrealizeObject(hPalette);
	    SelectPalette(hDC, hPalette, FALSE);
	    RealizePalette(hDC);
	    return TRUE;
	}
	return FALSE;

    case WM_CLOSE:
	PostQuitMessage(0);
	return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam); 
} 

HWND
CreateOpenGLWindow(char* title, int x, int y, int width, int height, 
		   BYTE type, DWORD flags)
{
    int         n, pf;
    HWND        hWnd;
    WNDCLASS    wc;
    LOGPALETTE* lpPal;
    PIXELFORMATDESCRIPTOR pfd;
    static HINSTANCE hInstance = 0;

    /* only register the window class once - use hInstance as a flag. */
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
	wc.lpszClassName = "OpenGL";

	if (!RegisterClass(&wc)) {
	    MessageBox(NULL, "RegisterClass() failed:  "
		       "Cannot register window class.", "Error", MB_OK);
	    return NULL;
	}
    }

    hWnd = CreateWindow("OpenGL", title, WS_OVERLAPPEDWINDOW |
			WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			x, y, width, height, NULL, NULL, hInstance, NULL);

    if (hWnd == NULL) {
	MessageBox(NULL, "CreateWindow() failed:  Cannot create a window.",
		   "Error", MB_OK);
	return NULL;
    }

    hDC = GetDC(hWnd);

    /* there is no guarantee that the contents of the stack that become
       the pfd are zeroed, therefore _make sure_ to clear these bits. */
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize        = sizeof(pfd);
    pfd.nVersion     = 1;
    pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | flags;
    pfd.iPixelType   = type;
    pfd.cDepthBits   = 32;
    pfd.cColorBits   = 32;

    pf = ChoosePixelFormat(hDC, &pfd);
    if (pf == 0) {
	MessageBox(NULL, "ChoosePixelFormat() failed:  "
		   "Cannot find a suitable pixel format.", "Error", MB_OK); 
	return 0;
    } 
 
    if (SetPixelFormat(hDC, pf, &pfd) == FALSE) {
	MessageBox(NULL, "SetPixelFormat() failed:  "
		   "Cannot set format specified.", "Error", MB_OK);
	return 0;
    } 

    DescribePixelFormat(hDC, pf, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

    if (pfd.dwFlags & PFD_NEED_PALETTE ||
	pfd.iPixelType == PFD_TYPE_COLORINDEX) {

	n = 1 << pfd.cColorBits;
	if (n > 256) n = 256;

	lpPal = (LOGPALETTE*)malloc(sizeof(LOGPALETTE) +
				    sizeof(PALETTEENTRY) * n);
	memset(lpPal, 0, sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * n);
	lpPal->palVersion = 0x300;
	lpPal->palNumEntries = n;

	GetSystemPaletteEntries(hDC, 0, n, &lpPal->palPalEntry[0]);
    
	/* if the pixel type is RGBA, then we want to make an RGB ramp,
	   otherwise (color index) set individual colors. */
	if (pfd.iPixelType == PFD_TYPE_RGBA) {
	    int redMask = (1 << pfd.cRedBits) - 1;
	    int greenMask = (1 << pfd.cGreenBits) - 1;
	    int blueMask = (1 << pfd.cBlueBits) - 1;
	    int i;

	    /* fill in the entries with an RGB color ramp. */
	    for (i = 0; i < n; ++i) {
		lpPal->palPalEntry[i].peRed = 
		    (((i >> pfd.cRedShift)   & redMask)   * 255) / redMask;
		lpPal->palPalEntry[i].peGreen = 
		    (((i >> pfd.cGreenShift) & greenMask) * 255) / greenMask;
		lpPal->palPalEntry[i].peBlue = 
		    (((i >> pfd.cBlueShift)  & blueMask)  * 255) / blueMask;
		lpPal->palPalEntry[i].peFlags = 0;
	    }
	} else {
	    lpPal->palPalEntry[0].peRed = 0;
	    lpPal->palPalEntry[0].peGreen = 0;
	    lpPal->palPalEntry[0].peBlue = 0;
	    lpPal->palPalEntry[0].peFlags = PC_NOCOLLAPSE;
	    lpPal->palPalEntry[1].peRed = 255;
	    lpPal->palPalEntry[1].peGreen = 0;
	    lpPal->palPalEntry[1].peBlue = 0;
	    lpPal->palPalEntry[1].peFlags = PC_NOCOLLAPSE;
	    lpPal->palPalEntry[2].peRed = 0;
	    lpPal->palPalEntry[2].peGreen = 255;
	    lpPal->palPalEntry[2].peBlue = 0;
	    lpPal->palPalEntry[2].peFlags = PC_NOCOLLAPSE;
	    lpPal->palPalEntry[3].peRed = 0;
	    lpPal->palPalEntry[3].peGreen = 0;
	    lpPal->palPalEntry[3].peBlue = 255;
	    lpPal->palPalEntry[3].peFlags = PC_NOCOLLAPSE;
	}

	hPalette = CreatePalette(lpPal);
	if (hPalette) {
	    SelectPalette(hDC, hPalette, FALSE);
	    RealizePalette(hDC);
	}

	free(lpPal);
    }

    ReleaseDC(hWnd, hDC);

    return hWnd;
}    

int APIENTRY
WinMain(HINSTANCE hCurrentInst, HINSTANCE hPreviousInst,
	LPSTR lpszCmdLine, int nCmdShow)
{
    HGLRC hRC;				/* opengl context */
    HWND  hWnd;				/* window */
    MSG   msg;				/* message */
    DWORD buffer = PFD_DOUBLEBUFFER;	/* buffering type */
    BYTE  color  = PFD_TYPE_RGBA;	/* color type */

    if (strstr(lpszCmdLine, "-sb")) {
	buffer = 0;
    } 
    if (strstr(lpszCmdLine, "-ci")) {
	color = PFD_TYPE_COLORINDEX;
    } 
    if (strstr(lpszCmdLine, "-h")) {
	MessageBox(NULL, "mouse [-ci] [-sb]\n"
		   "  -sb   single buffered\n"
		   "  -ci   color index\n",
		   "Usage help", MB_ICONINFORMATION);
	exit(0);
    }

    hWnd = CreateOpenGLWindow("mouse", 0, 0, 256, 256, color, buffer);
    if (hWnd == NULL)
	exit(1);

    hDC = GetDC(hWnd);
    hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);

    init();

    ShowWindow(hWnd, nCmdShow);

    while(GetMessage(&msg, hWnd, 0, 0)) {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }

    wglMakeCurrent(NULL, NULL);
    ReleaseDC(hWnd, hDC);
    wglDeleteContext(hRC);
    DestroyWindow(hWnd);
    if (hPalette)
	DeleteObject(hPalette);

    return msg.wParam;
}