// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for RayLib
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_x.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";

#include "raylib.h"

#include <stdlib.h>

#include <stdarg.h>
// #include <sys/time.h>
#include <sys/types.h>
// #include <sys/socket.h>

// #include <netinet/in.h>
// #include <errnos.h>
#include <signal.h>

#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

#include "doomdef.h"

#define POINTER_WARP_COUNTDOWN	1

typedef struct {
	unsigned char *data;
} XImage;

XImage*		image;
int		X_width;
int		X_height;

// XShmSegmentInfo	X_shminfo;
int		X_shmeventtype;

// Fake mouse handling.
// This cannot work properly w/o DGA.
// Needs an invisible mouse cursor at least.
boolean		grabMouse;
int		doPointerWarp = POINTER_WARP_COUNTDOWN;

// Blocky mode,
// replace each 320x200 pixel with multiply*multiply pixels.
// According to Dave Taylor, it still is a bonehead thing
// to use ....
static int	multiply=1;

static Color colors[256];

Image *buffer;
Texture2D texture;
RenderTexture2D target;

int lastX = 0;
int lastY = 0;
int data1 = 0;

//
//  Translates the key from raylib
//

int raylibkey(int key)
{
	int rc = 0;

    switch(key)
    {
		case KEY_A:
		case KEY_LEFT:			rc = KEY_LEFTARROW;		break;
		case KEY_D:
		case KEY_RIGHT:			rc = KEY_RIGHTARROW;	break;
		case KEY_S:
		case KEY_DOWN:			rc = KEY_DOWNARROW;		break;
		case KEY_W:
		case KEY_UP: 			rc = KEY_UPARROW;		break;
		case RAY_KEY_ESCAPE: 	rc = KEY_ESCAPE;		break;
		case RAY_KEY_ENTER:		rc = KEY_ENTER;			break;
		case KEY_LEFT_CONTROL:
		case KEY_RIGHT_CONTROL:
								rc = KEY_RCTRL;			break;
		case KEY_LEFT_SHIFT:
		case KEY_RIGHT_SHIFT:
								rc = KEY_RSHIFT;		break;
		case KEY_E:
		case RAY_KEY_SPACE:
								rc = KEY_SPACE;			break;
		case RAY_KEY_TAB:
								rc= KEY_TAB;			break;
	
		case RAY_KEY_F1:	rc = KEY_F1;		break;
		case RAY_KEY_F2:	rc = KEY_F2;		break;
		case RAY_KEY_F3:	rc = KEY_F3;		break;
		case RAY_KEY_F4:	rc = KEY_F4;		break;
		case RAY_KEY_F5:	rc = KEY_F5;		break;
		case RAY_KEY_F6:	rc = KEY_F6;		break;
		case RAY_KEY_F7:	rc = KEY_F7;		break;
		case RAY_KEY_F8:	rc = KEY_F8;		break;
		case RAY_KEY_F9:	rc = KEY_F9;		break;
		case RAY_KEY_F10:	rc = KEY_F10;		break;
		case RAY_KEY_F11:	rc = KEY_F11;		break;
		case RAY_KEY_F12:	rc = KEY_F12;		break;
		case KEY_LEFT_ALT:
		case KEY_RIGHT_ALT:
		case KEY_LEFT_SUPER:
		case KEY_RIGHT_SUPER:
		rc = KEY_RALT;
		break;

		case RAY_KEY_BACKSPACE:
		case KEY_DELETE:	rc = KEY_BACKSPACE;	break;
		case RAY_KEY_PAUSE:	rc = KEY_PAUSE;		break;

		case KEY_EQUAL:
		case KEY_KP_EQUAL:	rc = KEY_EQUALS;	break;

		case RAY_KEY_MINUS:
		case KEY_KP_SUBTRACT:	rc = KEY_MINUS;		break;

		if (rc >= RAY_KEY_SPACE && rc <= KEY_GRAVE)
			rc = rc - RAY_KEY_SPACE + ' ';
		if (rc >= 'A' && rc <= 'Z')
			rc = rc;
		break;
	}




    return rc;

}

void I_ShutdownGraphics(void)
{
	CloseWindow();
}

//
// I_StartFrame
//
void I_StartFrame (void)
{
    // er?

}

static int	lastmousex = 0;
static int	lastmousey = 0;
boolean		mousemoved = false;
boolean		shmFinished;

void I_GetEvent(void)
{
    event_t event;

	for (int i = 0; i < 512; i++) {
		boolean keyDown = IsKeyPressed(i);
		boolean keyUp = IsKeyReleased(i);

		if (keyDown) {
			event.type = ev_keydown;
			event.data1 = raylibkey(i);
			if (event.data1 > 0)
				D_PostEvent(&event);

		}
		else if(keyUp) {
			event.type = ev_keyup;
			event.data1 = raylibkey(i);
			if (event.data1 > 0)
				D_PostEvent(&event);
		}
	}
	
	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
		data1 |= 1;  // set mouse 1
	}
	else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
		data1 &= 6; // clear mouse 1
	}

	if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) {
		data1 |= 2;  // set mouse 1
	}
	else if (IsMouseButtonReleased(MOUSE_MIDDLE_BUTTON)) {
		data1 &= 5; // clear mouse 1
	}

	if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
		data1 |= 4;  // set mouse 1
	}
	else if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
		data1 &= 3; // clear mouse 1
	}

	event.type = ev_mouse;
	event.data1 = data1;
	event.data2 = (GetMouseX() - GetScreenWidth() / 2) * 2;
	event.data3 = (GetScreenHeight() / 2 - GetMouseY()) * 2;
	D_PostEvent(&event);


}

//
// I_StartTic
//
void I_StartTic (void)
{

	I_GetEvent();

	SetMousePosition(GetScreenWidth() / 2, GetScreenHeight() / 2);

}


//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
    // what is this?
}

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{

    static int	lasttic;
    int		tics;
    int		i;
    // UNUSED static unsigned char *bigscreen=0;

    // draws little dots on the bottom of the screen
    if (devparm)
    {

	i = I_GetTime();
	tics = i - lasttic;
	lasttic = i;
	if (tics > 20) tics = 20;

	for (i=0 ; i<tics*2 ; i+=2)
	    screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;
	for ( ; i<20*2 ; i+=2)
	    screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;
    
    }

    // scales the screen size before blitting it
    if (multiply == 2)
    {
	unsigned int *olineptrs[2];
	unsigned int *ilineptr;
	int x, y, i;
	unsigned int twoopixels;
	unsigned int twomoreopixels;
	unsigned int fouripixels;

	ilineptr = (unsigned int *) (screens[0]);
	for (i=0 ; i<2 ; i++)
	    olineptrs[i] = (unsigned int *) &image->data[i*X_width];

	y = SCREENHEIGHT;
	while (y--)
	{
	    x = SCREENWIDTH;
	    do
	    {
		fouripixels = *ilineptr++;
		twoopixels =	(fouripixels & 0xff000000)
		    |	((fouripixels>>8) & 0xffff00)
		    |	((fouripixels>>16) & 0xff);
		twomoreopixels =	((fouripixels<<16) & 0xff000000)
		    |	((fouripixels<<8) & 0xffff00)
		    |	(fouripixels & 0xff);
#ifdef __BIG_ENDIAN__
		*olineptrs[0]++ = twoopixels;
		*olineptrs[1]++ = twoopixels;
		*olineptrs[0]++ = twomoreopixels;
		*olineptrs[1]++ = twomoreopixels;
#else
		*olineptrs[0]++ = twomoreopixels;
		*olineptrs[1]++ = twomoreopixels;
		*olineptrs[0]++ = twoopixels;
		*olineptrs[1]++ = twoopixels;
#endif
	    } while (x-=4);
	    olineptrs[0] += X_width/4;
	    olineptrs[1] += X_width/4;
	}

    }
    else if (multiply == 3)
    {
	unsigned int *olineptrs[3];
	unsigned int *ilineptr;
	int x, y, i;
	unsigned int fouropixels[3];
	unsigned int fouripixels;

	ilineptr = (unsigned int *) (screens[0]);
	for (i=0 ; i<3 ; i++)
	    olineptrs[i] = (unsigned int *) &image->data[i*X_width];

	y = SCREENHEIGHT;
	while (y--)
	{
	    x = SCREENWIDTH;
	    do
	    {
		fouripixels = *ilineptr++;
		fouropixels[0] = (fouripixels & 0xff000000)
		    |	((fouripixels>>8) & 0xff0000)
		    |	((fouripixels>>16) & 0xffff);
		fouropixels[1] = ((fouripixels<<8) & 0xff000000)
		    |	(fouripixels & 0xffff00)
		    |	((fouripixels>>8) & 0xff);
		fouropixels[2] = ((fouripixels<<16) & 0xffff0000)
		    |	((fouripixels<<8) & 0xff00)
		    |	(fouripixels & 0xff);
#ifdef __BIG_ENDIAN__
		*olineptrs[0]++ = fouropixels[0];
		*olineptrs[1]++ = fouropixels[0];
		*olineptrs[2]++ = fouropixels[0];
		*olineptrs[0]++ = fouropixels[1];
		*olineptrs[1]++ = fouropixels[1];
		*olineptrs[2]++ = fouropixels[1];
		*olineptrs[0]++ = fouropixels[2];
		*olineptrs[1]++ = fouropixels[2];
		*olineptrs[2]++ = fouropixels[2];
#else
		*olineptrs[0]++ = fouropixels[2];
		*olineptrs[1]++ = fouropixels[2];
		*olineptrs[2]++ = fouropixels[2];
		*olineptrs[0]++ = fouropixels[1];
		*olineptrs[1]++ = fouropixels[1];
		*olineptrs[2]++ = fouropixels[1];
		*olineptrs[0]++ = fouropixels[0];
		*olineptrs[1]++ = fouropixels[0];
		*olineptrs[2]++ = fouropixels[0];
#endif
	    } while (x-=4);
	    olineptrs[0] += 2*X_width/4;
	    olineptrs[1] += 2*X_width/4;
	    olineptrs[2] += 2*X_width/4;
	}

    }
    else if (multiply == 4)
    {
	// Broken. Gotta fix this some day.
	void Expand4(unsigned *, double *);
  	Expand4 ((unsigned *)(screens[0]), (double *) (image->data));
    }


	#define min(a, b) ((a)<(b)? (a) : (b))
	float scale = min((float)GetScreenWidth()/SCREENWIDTH, (float)GetScreenHeight()/SCREENHEIGHT);
	int fps = GetFPS();
	char fpsString[5];
	snprintf(fpsString, 5, "%d", fps);


	BeginDrawing();
		
		BeginTextureMode(target);
			for (int i = 0; i < SCREENWIDTH * SCREENHEIGHT; i++) {
				Color c = colors[image->data[i]];
				if (c.a != 255) {
					printf("%d\n", image->data[i]);
				}
				DrawPixel(i % SCREENWIDTH, (int) i / SCREENWIDTH, c);
			}
		EndTextureMode();

		// Draw RenderTexture2D to window, properly scaled
		DrawTexturePro(target.texture, (Rectangle){ 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height },
			(Rectangle){ (GetScreenWidth() - ((float)SCREENWIDTH * scale)) * 0.5, (GetScreenHeight() - ((float)SCREENHEIGHT * scale)) * 0.5,
			(float)SCREENWIDTH * scale, (float)SCREENHEIGHT * scale }, (Vector2){ 0, 0 }, 0.0f, WHITE);
	

		DrawText(fpsString, GetScreenWidth() - (strlen(fpsString) - 1) * 25, 0, 20, WHITE);
	EndDrawing();

}


//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}



// Palette stuff.
void UploadNewPalette(byte *palette)
{

    register int	i;
    register int	c;
    static boolean	firstcall = true;

	// initialize the colormap
	if (firstcall)
	{
		firstcall = false;
		for (i=0 ; i<256 ; i++)
		{
			// colors[i].pixel = i;
			// colors[i].flags = DoRed|DoGreen|DoBlue;
			colors[i] = PINK;
			colors[i].a = 255;
		}
	}

	// set the X colormap entries
	for (i = 0; i < 256; i++)
	{
		c = gammatable[usegamma][*palette++];
		colors[i].r = (c<<8) + c;
		c = gammatable[usegamma][*palette++];
		colors[i].g = (c<<8) + c;
		c = gammatable[usegamma][*palette++];
		colors[i].b = (c<<8) + c;

		colors[i].a = 255;
	
	}

	
}

//
// I_SetPalette
//
void I_SetPalette (byte* palette)
{
    UploadNewPalette(palette);
}

void I_InitGraphics(void)
{

    char*		displayname;
    char*		d;
    int			n;
    int			pnum;
    int			x=0;
    int			y=0;
    
    // warning: char format, different type arg
    char		xsign=' ';
    char		ysign=' ';
    
    int			oktodraw;
    unsigned long	attribmask;
    // XSetWindowAttributes attribs;
    // XGCValues		xgcvalues;
    int			valuemask;
    static int		firsttime=1;

    if (!firsttime)
	return;
    firsttime = 0;

    signal(SIGINT, (void (*)(int)) I_Quit);

    if (M_CheckParm("-2"))
	multiply = 2;

    if (M_CheckParm("-3"))
	multiply = 3;

    if (M_CheckParm("-4"))
	multiply = 4;

    X_width = SCREENWIDTH * multiply;
    X_height = SCREENHEIGHT * multiply;

    // check for command-line display name
    if ( (pnum=M_CheckParm("-disp")) ) // suggest parentheses around assignment
	displayname = myargv[pnum+1];
    else
	displayname = 0;

    // check if the user wants to grab the mouse (quite unnice)
    grabMouse = !!M_CheckParm("-grabmouse");

    // check for command-line geometry
    if ( (pnum=M_CheckParm("-geom")) ) // suggest parentheses around assignment
    {
	// warning: char format, different type arg 3,5
	n = sscanf(myargv[pnum+1], "%c%d%c%d", &xsign, &x, &ysign, &y);
	
	if (n==2)
	    x = y = 0;
	else if (n==6)
	{
	    if (xsign == '-')
		x = -x;
	    if (ysign == '-')
		y = -y;
	}
	else
	    I_Error("bad -geom parameter");
    }

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);  
	InitWindow(960, 600, "raylib-doom");
	SetExitKey(0);
	DisableCursor();

	image = malloc(sizeof(XImage));
	image->data = malloc(SCREENWIDTH * SCREENHEIGHT);
	screens[0] = (unsigned char *) (image->data);
	
	buffer = malloc(sizeof(Image));

	buffer->width = SCREENWIDTH;
	buffer->height = SCREENHEIGHT;
	buffer->mipmaps = 1;
	buffer->format = UNCOMPRESSED_R8G8B8A8;
	buffer->data = (unsigned char *) malloc(buffer->width * buffer->height * 4);

	// Render texture initialization, used to hold the rendering result so we can easily resize it
    target = LoadRenderTexture(SCREENWIDTH, SCREENHEIGHT);
    SetTextureFilter(target.texture, FILTER_POINT);  // Texture scale filter to use


    // if (multiply == 1)
	// screens[0] = (unsigned char *) (image->data);
    // else
	// screens[0] = (unsigned char *) malloc (SCREENWIDTH * SCREENHEIGHT);
}


unsigned	exptable[256];

void InitExpand (void)
{
    int		i;
	
    for (i=0 ; i<256 ; i++)
	exptable[i] = i | (i<<8) | (i<<16) | (i<<24);
}

double		exptable2[256*256];

void InitExpand2 (void)
{
    int		i;
    int		j;
    // UNUSED unsigned	iexp, jexp;
    double*	exp;
    union
    {
	double 		d;
	unsigned	u[2];
    } pixel;
	
    printf ("building exptable2...\n");
    exp = exptable2;
    for (i=0 ; i<256 ; i++)
    {
	pixel.u[0] = i | (i<<8) | (i<<16) | (i<<24);
	for (j=0 ; j<256 ; j++)
	{
	    pixel.u[1] = j | (j<<8) | (j<<16) | (j<<24);
	    *exp++ = pixel.d;
	}
    }
    printf ("done.\n");
}

int	inited;

void
Expand4
( unsigned*	lineptr,
  double*	xline )
{
    double	dpixel;
    unsigned	x;
    unsigned 	y;
    unsigned	fourpixels;
    unsigned	step;
    double*	exp;
	
    exp = exptable2;
    if (!inited)
    {
	inited = 1;
	InitExpand2 ();
    }
		
		
    step = 3*SCREENWIDTH/2;
	
    y = SCREENHEIGHT-1;
    do
    {
	x = SCREENWIDTH;

	do
	{
	    fourpixels = lineptr[0];
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[0] = dpixel;
	    xline[160] = dpixel;
	    xline[320] = dpixel;
	    xline[480] = dpixel;
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	    xline[1] = dpixel;
	    xline[161] = dpixel;
	    xline[321] = dpixel;
	    xline[481] = dpixel;

	    fourpixels = lineptr[1];
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[2] = dpixel;
	    xline[162] = dpixel;
	    xline[322] = dpixel;
	    xline[482] = dpixel;
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	    xline[3] = dpixel;
	    xline[163] = dpixel;
	    xline[323] = dpixel;
	    xline[483] = dpixel;

	    fourpixels = lineptr[2];
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[4] = dpixel;
	    xline[164] = dpixel;
	    xline[324] = dpixel;
	    xline[484] = dpixel;
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	    xline[5] = dpixel;
	    xline[165] = dpixel;
	    xline[325] = dpixel;
	    xline[485] = dpixel;

	    fourpixels = lineptr[3];
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff0000)>>13) );
	    xline[6] = dpixel;
	    xline[166] = dpixel;
	    xline[326] = dpixel;
	    xline[486] = dpixel;
			
	    dpixel = *(double *)( (int)exp + ( (fourpixels&0xffff)<<3 ) );
	    xline[7] = dpixel;
	    xline[167] = dpixel;
	    xline[327] = dpixel;
	    xline[487] = dpixel;

	    lineptr+=4;
	    xline+=8;
	} while (x-=16);
	xline += step;
    } while (y--);
}


