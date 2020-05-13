#include <windows.h>		/* must include this before GL/gl.h */
#include <GL/gl.h>			/* OpenGL header file */
#include <GL/glu.h>			/* OpenGL utilities header file */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define clamp(x) x = x > 360.0f ? x-360.0f : x < -360.0f ? x+=360.0f : x
#define set_vec3(v,i,j,k,op) v.x op i; v.y op j; v.z op k;
#define unpack(d) d.x, d.y, d.z

enum { 
    PAN = 1,			/* pan state bit */
    ROTATE,				/* rotate state bits */
    ZOOM				/* zoom state bit */
};

enum {
	XMODE,
	YMODE,
	ZMODE
};

struct GLDot {
	GLfloat x,y,z;
};


struct GLSide {
	struct GLDot tl, tr, br, bl;
	GLfloat r, g, b;
};

struct GLCubeBlock {
	struct GLDot base;
	struct GLSide *sides[3];
};

struct GLCubeSide {
	struct GLDot vec;
	GLint mode;
	struct GLCubeBlock *blocks[3][3];
};

struct GLCubeSide SIDES[6];
struct GLDot CUBE[8] = {{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
			{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
GLboolean CUBE_INITIALIZED = GL_FALSE;

HDC hDC;						/* device context */
HPALETTE hPalette = 0;			/* custom palette (if needed) */
GLfloat trans[3];				/* current translation */
GLfloat rot[2];					/* current rotation */
GLboolean animate = GL_TRUE;	/* animate flag */

void swap(struct GLCubeBlock *a, struct GLCubeBlock *b)
{
	struct GLCubeBlock t = *a;
	*a = *b;
	*b = t;
}

void transposeCubeMatrix(struct GLCubeBlock *mtrx[3][3][3])
{
	for (int i=0; i<3; i++) {
		swap(mtrx[i][0][1],mtrx[i][1][0]);
		swap(mtrx[i][0][2],mtrx[i][2][0]);
		swap(mtrx[i][1][2],mtrx[i][2][1]);
	}
}

GLfloat glRandf() {
	return (GLfloat)rand()/(GLfloat)RAND_MAX;
}

void drawCubeSide(struct GLSide* side) {
	glBegin(GL_TRIANGLE_STRIP);
		glColor3f(fabsf(side->r), fabsf(side->g), fabsf(side->b));
		glVertex3f(unpack(side->tl));
		glVertex3f(unpack(side->tr));
		glVertex3f(unpack(side->bl));
		glVertex3f(unpack(side->br));
	glEnd();
}

void processCubeColors() {
	// for (int i=0; i<6; i++) {
	// 	Sides[i].r += 0.00001f; Sides[i].r -= Sides[i].r>1 ? 2.0f: 0.0f;
	// 	Sides[i].g += 0.00001f; Sides[i].g -= Sides[i].g>1 ? 2.0f: 0.0f;
	// 	Sides[i].b += 0.00001f; Sides[i].b -= Sides[i].b>1 ? 2.0f: 0.0f;
	// }
}

void debug_print_sides()
{
	for (int i=0; i<6; i++) {
		printf("Side[%d]:\n", i);
		printf("Vec norm: %f %f %f\n", SIDES[i].vec.x, SIDES[i].vec.y, SIDES[i].vec.z);
		for (int j=0; j<3; j++) {
			for (int z=0; z<3; z++) {
				printf("POINT %f %f %f\n", unpack(SIDES[i].blocks[j][z]->base));
			}
		}
		printf("\n");
	}
}

struct GLSide *createSurface(GLint mode, GLfloat width, struct GLDot base, GLfloat r, GLfloat g, GLfloat b)
{
	struct GLSide *side = malloc(sizeof(struct GLSide));
	side->r = r;
	side->g = g;
	side->b = b;
	switch (mode)
	{
		case XMODE:
			set_vec3(side->tl,base.x, base.y-width/2, base.z+width/2, =);
			set_vec3(side->tr,base.x, base.y+width/2, base.z+width/2, =);
			set_vec3(side->br,base.x, base.y+width/2, base.z-width/2, =);
			set_vec3(side->bl,base.x, base.y-width/2, base.z-width/2, =); 
			break;
		case YMODE:
			set_vec3(side->tl,base.x-width/2, base.y, base.z+width/2, =);
			set_vec3(side->tr,base.x+width/2, base.y, base.z+width/2, =);
			set_vec3(side->br,base.x+width/2, base.y, base.z-width/2, =);
			set_vec3(side->bl,base.x-width/2, base.y, base.z-width/2, =);
			break;
		case ZMODE:
			set_vec3(side->tl,base.x-width/2, base.y+width/2, base.z, =);
			set_vec3(side->tr,base.x+width/2, base.y+width/2, base.z, =);
			set_vec3(side->br,base.x+width/2, base.y-width/2, base.z, =);
			set_vec3(side->bl,base.x-width/2, base.y-width/2, base.z, =);
			break;
	}
	return side;
}

void initCubeSide(struct GLCubeSide *side, GLfloat width, GLfloat r, GLfloat g, GLfloat b)
{
	printf("Initializing surface cube side (%f, %f, %f):\n", unpack(side->vec));
	for (int i=0; i<3; i++) {
		for (int j=0; j<3; j++) {
			struct GLDot surfaceBase;
			surfaceBase.x = side->blocks[i][j]->base.x + side->vec.x * width/2;
			surfaceBase.y = side->blocks[i][j]->base.y + side->vec.y * width/2;
			surfaceBase.z = side->blocks[i][j]->base.z + side->vec.z * width/2;
			side->blocks[i][j]->sides[side->mode] = createSurface(side->mode, width, surfaceBase, r, g, b);
			printf("Initialized block (%f, %f, %f)\n", unpack(side->blocks[i][j]->base));
			printf("with  surface tl-(%f,%f,%f)\ntr-(%f,%f,%f)\nbr-(%f,%f,%f)\nbl-(%f,%f,%f)\n\n", unpack(side->blocks[i][j]->sides[side->mode]->tl),
																			unpack(side->blocks[i][j]->sides[side->mode]->tr),
																			unpack(side->blocks[i][j]->sides[side->mode]->br),
																			unpack(side->blocks[i][j]->sides[side->mode]->bl));
		}
	}
	printf("\n");
}

void initCube() {
	// Let's get all possible GLCubeBlocks with the base from our main rubicks cube 0,0,0

	struct GLCubeBlock *mtrx[3][3][3];
	printf("Initializing small cubes' base\n");
	for (int y=-1; y<2; y++) {
		for (int z=-1; z<2; z++) {
			for (int x=-1; x<2; x++){
				printf("Got point %d %d %d\n", y, z, x);
				struct GLCubeBlock *b = malloc(sizeof(struct GLCubeBlock));
				b->base.x = (GLfloat)x; b->base.y = (GLfloat)y; b->base.z = (GLfloat)z;
				mtrx[y+1][z+1][x+1] = b;
			}
		}
	}

	// Now we need to combine sides of our rubick's cube
	// TOP and BOTTOM
	// mtrx[0] and mtrx[2] <=> SIDES[0] and SIDES[1]
	int s = 0;
	SIDES[0].mode = YMODE;
	SIDES[1].mode = YMODE;
	set_vec3(SIDES[0].vec, 0, -1, 0, =);
	set_vec3(SIDES[1].vec, 0, 1, 0, =);
	for (int k=0; k<3; k+=2) {
		for (int i=0; i<3; i++) {
			for (int j=0; j<3; j++) {
				SIDES[s].blocks[i][j] = mtrx[k][i][j];
			}
		}
		s++;
	}
	
	// FRONT and BACK
	// mtrx[...][0][...] and mtrx[...][2][...] <=> SIDES[2] and SIDES[3]
	SIDES[2].mode = ZMODE;
	SIDES[3].mode = ZMODE;
	set_vec3(SIDES[2].vec, 0, 0, -1, =);
	set_vec3(SIDES[3].vec, 0, 0, 1, =); 
	for (int k=0; k<3; k+=2) {
		for (int i=0; i<3; i++) {
			for (int j=0; j<3; j++) {
				SIDES[s].blocks[i][j] = mtrx[i][j][k];
			}
		}
		s++;
	}

	// LEFT and RIGHT
	// (after matrix transpose)
	// mtrx[...][0][...] and mtrx[...][2][...] <=> SIDES[4] and SIDES[5]
	SIDES[4].mode = XMODE;
	SIDES[5].mode = XMODE;
	set_vec3(SIDES[4].vec, -1, 0, 0, =);
	set_vec3(SIDES[5].vec, 1, 0, 0, =);
	transposeCubeMatrix(mtrx);
	for (int k=0; k<3; k+=2) {
		for (int i=0; i<3; i++) {
			for (int j=0; j<3; j++) {
				SIDES[s].blocks[i][j] = mtrx[i][k][j];
			}
		}
		s++;
	}

	debug_print_sides();

	// Initializing surface for each block on each cube side
	initCubeSide(&SIDES[0], 0.8, 1.0, 0.0, 0.0); // Red side
	initCubeSide(&SIDES[1], 0.8, 0.0, 1.0, 0.0); // Green side
	// ???
	initCubeSide(&SIDES[2], 0.8, 0.0, 0.0, 1.0); // Blue side
	initCubeSide(&SIDES[3], 0.8, 1.0, 1.0, 0.0); // Yellow side
	// ???
	initCubeSide(&SIDES[4], 0.8, 1.0, 0.0, 1.0); // Purple side
	initCubeSide(&SIDES[5], 0.8, 1.0, 1.0, 1.0); // White side

	// Let's initialize list for each side
	// // Init render list
	// glNewList(1, GL_COMPILE);
	// for (int i=0; i<6; i++) {
	// 	drawCubeSide(&Sides[i]);
	// }
	// glEndList();

	CUBE_INITIALIZED = GL_TRUE;
}

static void update(int state, int ox, int nx, int oy, int ny)
{
	int dx = ox - nx;
	int dy = ny - oy;

	printf("Update animation: dx - %d, dy - %d\n", dx, dy);

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


void init()
{
    glEnable(GL_DEPTH_TEST);
}

void reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (float)width/height, 0.001, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -3.0f);
}

void display()
{
    /* rotate cube*/
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();
    glTranslatef(trans[0], trans[1], trans[2]);
	
    glRotatef(rot[0], 1.0f, 0.0f, 0.0f);
    glRotatef(rot[1], 0.0f, 1.0f, 0.0f);

	/* init cube if it isn't*/
	if (!CUBE_INITIALIZED) {initCube();}

	/* process animation */
	if (animate) {
		rot[0] += 0.002f;
		rot[1] += 0.002f;
		processCubeColors();
	}

	/* draw updated sides of cube, call render list 1 */
	//glCallList(1);
	for (int i=0; i<6; i++) {
		for (int j=0; j<3; j++) {
			for (int k=0; k<3; k++) {
				drawCubeSide(SIDES[i].blocks[j][k]->sides[SIDES[i].mode]);
			}
		}
	}

    glPopMatrix();
    glFlush();
    SwapBuffers(hDC);			/* nop if singlebuffered */
}


LONG WINAPI
WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{ 
    static PAINTSTRUCT ps;
    static GLboolean left = GL_FALSE;	/* left button currently down? */
    static GLboolean right = GL_FALSE;	/* right button currently down? */
    static GLuint state = 0;			/* mouse state flag */
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
			printf("You've pressed char %d\n", (int)wParam);
			switch (wParam) {
				case 27:			/* ESC key */
					PostQuitMessage(0);
					break;
				case 32:
					printf("animate - %d\n", animate);
					animate ^= GL_TRUE;
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

    hWnd = CreateOpenGLWindow("mouse", 200, 200, 800, 600, color, buffer);
    if (hWnd == NULL)
		exit(1);

    hDC = GetDC(hWnd);
    hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);

    init();

    ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

    while (1) {
	while(PeekMessage(&msg, hWnd, 0, 0, PM_NOREMOVE)) {
	    if(GetMessage(&msg, hWnd, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	    } else {
		goto quit;
	    }
	}
	display();
    }

	quit:
		wglMakeCurrent(NULL, NULL);
		ReleaseDC(hWnd, hDC);
		wglDeleteContext(hRC);
		DestroyWindow(hWnd);
		if (hPalette)
			DeleteObject(hPalette);

    return 0;
}