/*
    xabsgui.cpp: abstract user interface for X11


    flexemu, an MC6809 emulator running FLEX
    Copyright (C) 1997-2004  W. Schwotzer

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include <misc1.h>
#ifdef ultrix
#include <errno.h>
#endif
#include <limits.h>

#ifdef UNIX
// autoconf adaption for sys/wait.h
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif
#endif // ifdef UNIX

#include "xabsgui.h"

#ifdef HAVE_X11

#include "e2video.h"
#include "mc6809.h"
#include "inout.h"
#include "schedule.h"

/* ARGSUSED */
void XAbstractGui::initialize(struct sGuiOptions *pOptions)
{
	AbstractGui::initialize(pOptions);
	bp_input[0] = 0;
	bp_input[1] = 0;
	visual = NULL;
	depth = 0;
	own_cmap = 0;
	copy_block = NULL;
}

Display *XAbstractGui::getDisplay(void)
{
	return (Display *)NULL;
}

Window XAbstractGui::getWindow(tWindowType)
{
	return (Window)0;
}

void XAbstractGui::set_cursor(int type /* = FLX_DEFAULT_CURSOR */)
{
	Display *dpy;
			
	dpy = getDisplay();

	switch (type)
	{
	case FLX_DEFAULT_CURSOR:
		XUndefineCursor(dpy, getWindow());
		break;
	case FLX_INVISIBLE_CURSOR:
	default:
	   if (cursor == None)
	   {
		Pixmap cursormask;
		XGCValues xgc;
		XColor dummycolour;
		GC gc;
			
		cursormask = XCreatePixmap (dpy, getWindow(), 1, 1, 1);
		xgc.function = GXclear;
		gc = XCreateGC (dpy, cursormask, GCFunction, &xgc);
		XFillRectangle (dpy, cursormask, gc, 0, 0, 1, 1);
		dummycolour.pixel = 0;
		dummycolour.red   = 0;
		dummycolour.green = 0;
		dummycolour.blue  = 0;
		dummycolour.flags = DoBlue;
		cursor = XCreatePixmapCursor (dpy, cursormask, cursormask,
			&dummycolour, &dummycolour, 0, 0);
		XFreeGC (dpy, gc);
		XFreePixmap (dpy, cursormask);
	   }
	   XDefineCursor(dpy, getWindow(), cursor);
	   break;
	}
}

char *XAbstractGui::get_title(void)
{
	if (cursor_type == FLX_DEFAULT_CURSOR)
		return PROGRAMNAME " V" VERSION " - Press SHIFT F10 to capture mouse";
	else
		return PROGRAMNAME " V" VERSION " - Press SHIFT F10 to release mouse";
}

void XAbstractGui::toggle_mouse_capture()
{
	XTextProperty titleProperty;
	Display       *dpy;
	char          *title;

	dpy = getDisplay();
	cursor_type = (cursor_type == FLX_DEFAULT_CURSOR) ?
	    FLX_INVISIBLE_CURSOR : FLX_DEFAULT_CURSOR;
	title = get_title();
	if (XStringListToTextProperty(&title, 1, &titleProperty) != 0)
		XSetWMName(dpy, getWindow(FLX_MAIN), &titleProperty);
	if (cursor_type == FLX_DEFAULT_CURSOR)
	{
		XUngrabPointer(dpy, CurrentTime);
	} else
	{
		unsigned int event_mask;

		event_mask = ButtonPressMask | ButtonReleaseMask |
			EnterWindowMask | LeaveWindowMask | PointerMotionMask |
			PointerMotionHintMask | Button1MotionMask |
			Button2MotionMask | Button3MotionMask |
			Button4MotionMask | Button5MotionMask |
			ButtonMotionMask | KeymapStateMask;
		XGrabPointer(dpy, getWindow(), True, event_mask,
			GrabModeAsync, GrabModeAsync, getWindow(),
			None, CurrentTime);
	}
	set_cursor(cursor_type);
}

void XAbstractGui::release_mouse_capture()
{
	cursor_type = FLX_INVISIBLE_CURSOR;
	toggle_mouse_capture();
}

void XAbstractGui::update_block(int block_number)
{
	if (memory->changed[block_number]) {
		memory->changed[block_number] = false;
		if (nColors > 2)
			update_color_block(block_number);
		else
			update_bw_block(block_number);
	}
}

void XAbstractGui::c_focusIn(XEvent *pevent)
{
}

void XAbstractGui::c_focusOut(XEvent *pevent)
{
	release_mouse_capture();
}

void XAbstractGui::update_color_block(int block_number)
{
	Display 	*dpy;
	Window	 	 win;
	int		 i;
	XImage		*img;
	Byte		*src;

	dpy = getDisplay();
	win = getWindow();
	if (e2video->vico1 & 0x01)
		src = memory->vram_ptrs[0x08] + block_number * YBLOCK_SIZE;
	else
		src = memory->vram_ptrs[0x0C] + block_number * YBLOCK_SIZE;
	img = image6[guiXSize-1][guiYSize-1];
	CopyToZPixmap(block_number, (Byte *)img->data, src, depth,
		(unsigned long *)pen);
	
	if (!(e2video->vico1 & 0x02)) {
		if (block_number == e2video->divided_block) {
			// first half display on the bottom of the window
			XPutImage(dpy, win, e2gc,
				img, 0, 0, 0, (WINDOWHEIGHT -
				(e2video->vico2 % BLOCKHEIGHT)) * guiYSize,
				BLOCKWIDTH * guiXSize,
				(e2video->vico2 % BLOCKHEIGHT) * guiYSize);
			// second half display on the top of window
			XPutImage(dpy, win, e2gc,
				img, 0, (e2video->vico2 % YBLOCKS) * guiYSize,
				0, 0, BLOCKWIDTH * guiXSize, (BLOCKHEIGHT -
				   (e2video->vico2 % BLOCKHEIGHT)) * guiYSize);
		} else {
			XPutImage(dpy, win, e2gc,
				img, 0, 0, 0,
				((block_number * BLOCKHEIGHT + WINDOWHEIGHT -
				   e2video->vico2) % WINDOWHEIGHT) * guiYSize,
				BLOCKWIDTH * guiXSize, BLOCKHEIGHT * guiYSize);
		} // else
	} else {
		for (i = 0; i < YBLOCK_SIZE * guiXSize * guiYSize; i++)
			copy_block[i] = pOptions->inverse ? 0x00 : 0xff;
		// allways display an empty screen
			XPutImage(dpy, win, e2gc,
				image1[guiXSize-1][guiYSize-1], 0, 0, 0,
				block_number * BLOCKHEIGHT * guiXSize,
				BLOCKWIDTH * guiXSize, BLOCKHEIGHT * guiYSize);
	} // else
}

void XAbstractGui::update_bw_block(int block_number)
{
	Display 	*dpy;
	Window	 	 win;
	int		 i;
	XImage		*img;
	Byte		*src;

	dpy = getDisplay();
	win = getWindow();
	if (e2video->vico1 & 0x01)
		src = memory->vram_ptrs[0x08] + block_number * YBLOCK_SIZE;
	else
		src = memory->vram_ptrs[0x0C] + block_number * YBLOCK_SIZE;
	img = image1[guiXSize-1][guiYSize-1];

	switch ((guiXSize << 4) | guiYSize) {
		int	j;
	  case 0x11: {// single width, single height
		Byte *btrg = copy_block;
		memcpy(btrg, src, RASTERLINE_SIZE * BLOCKHEIGHT);
	  	break; }
	  case 0x21: {// double width, single height
		Word *wtrg = (Word *)copy_block;
		for (i = 0; i < BLOCKHEIGHT; i++) {
			for (j = 0; j < RASTERLINE_SIZE; j++) {
				*(wtrg++) = conv_2byte_tab[*(src++)];
			}
		}
	  	break; }
	  case 0x22: {// double width, double height
		Word *wtrg = (Word *)copy_block;
		for (i = 0; i < BLOCKHEIGHT; i++) {
			for (j = 0; j < RASTERLINE_SIZE; j++) {
				*wtrg = *(wtrg+RASTERLINE_SIZE) =
					conv_2byte_tab[*(src++)];
				wtrg++;
			}
			wtrg += RASTERLINE_SIZE;
		}
		break; }
	  case 0x23: { // double width, tripple height
		Word *wtrg = (Word *)copy_block;
		for (i = 0; i < BLOCKHEIGHT; i++) {
			for (j = 0; j < RASTERLINE_SIZE; j++) {
				*wtrg = *(wtrg+RASTERLINE_SIZE) =
				  *(wtrg+2*RASTERLINE_SIZE) =
				  	conv_2byte_tab[*(src++)];
				wtrg++;
			}
			wtrg += 2 * RASTERLINE_SIZE;
		}
		break; }
	  case 0x24: { // double width, quadruple height
		Word *wtrg = (Word *)copy_block;
		for (i = 0; i < BLOCKHEIGHT; i++) {
			for (j = 0; j < RASTERLINE_SIZE; j++) {
				*wtrg = *(wtrg+RASTERLINE_SIZE) =
				  *(wtrg+2*RASTERLINE_SIZE) =
				  *(wtrg+3*RASTERLINE_SIZE) =
				  	conv_2byte_tab[*(src++)];
				wtrg++;
			}
			wtrg += 3 * RASTERLINE_SIZE;
		}
		break; }
	  case 0x12:   // single width, double height
	  case 0x13:   // single width, tripple height
	  case 0x14: { // single width, quadruple height
		Byte *btrg = copy_block;
		for (i = 0; i < BLOCKHEIGHT; i++) {
			for (j = 0; j < guiYSize; j++) {
				memcpy(btrg, src, RASTERLINE_SIZE);
				btrg += RASTERLINE_SIZE;
			}
			src += RASTERLINE_SIZE;
		}
	  	break; }
	  case 0x31: { // triple width, single height
		Byte *btrg = copy_block;
		DWord pattern;
		for (i = 0; i < BLOCKHEIGHT; i++) {
			for (j = 0; j < RASTERLINE_SIZE; j++) {
				pattern = conv_3byte_tab[*(src++)];
				*(btrg++) = (pattern & 0xFF0000) >> 16;
				*(btrg++) = (pattern & 0xFF00) >> 8;
				*(btrg++) = (pattern & 0xFF);
			}
		}
		break; }
	  case 0x32: { // triple width, double height
		Byte *btrg = copy_block;
		DWord pattern;
		for (i = 0; i < BLOCKHEIGHT; i++) {
			for (j = 0; j < RASTERLINE_SIZE; j++) {
				pattern = conv_3byte_tab[*(src++)];
				*btrg = *(btrg + 3 * RASTERLINE_SIZE)
					= (pattern & 0xFF0000) >> 16;
				*(btrg+1) = *(btrg + 3*RASTERLINE_SIZE + 1)
					= (pattern & 0xFF00) >> 8;
				*(btrg+2) = *(btrg + 3*RASTERLINE_SIZE + 2)
					= (pattern & 0xFF);
				btrg += 3;
			}
			btrg += 3 * RASTERLINE_SIZE;
		}
		break; }
	  case 0x33: { // triple width, triple height
		Byte *btrg = copy_block;
		DWord pattern;
		for (i = 0; i < BLOCKHEIGHT; i++) {
			for (j = 0; j < RASTERLINE_SIZE; j++) {
				pattern = conv_3byte_tab[*(src++)];
				*btrg =
				*(btrg + 3 * RASTERLINE_SIZE) =
				*(btrg + 6 * RASTERLINE_SIZE) =
					(pattern & 0xFF0000) >> 16;
				*(btrg+1) =
				*(btrg + 3*RASTERLINE_SIZE + 1) =
				*(btrg + 6*RASTERLINE_SIZE + 1) =
					(pattern & 0xFF00) >> 8;
				*(btrg+2) =
				*(btrg + 3*RASTERLINE_SIZE + 2) =
				*(btrg + 6*RASTERLINE_SIZE + 2) =
					(pattern & 0xFF);
				btrg += 3;
			}
			btrg += 6 * RASTERLINE_SIZE;
		}
		break; }
	  case 0x34: { // triple width, quadruple height
		Byte *btrg = copy_block;
		DWord pattern;
		for (i = 0; i < BLOCKHEIGHT; i++) {
			for (j = 0; j < RASTERLINE_SIZE; j++) {
				pattern = conv_3byte_tab[*(src++)];
				*btrg =
				*(btrg + 3 * RASTERLINE_SIZE) =
				*(btrg + 6 * RASTERLINE_SIZE) =
				*(btrg + 9 * RASTERLINE_SIZE) =
					(pattern & 0xFF0000) >> 16;
				*(btrg+1) =
				*(btrg + 3*RASTERLINE_SIZE + 1) =
				*(btrg + 6*RASTERLINE_SIZE + 1) =
				*(btrg + 9*RASTERLINE_SIZE + 1) =
					(pattern & 0xFF00) >> 8;
				*(btrg+2) =
				*(btrg + 3*RASTERLINE_SIZE + 2) =
				*(btrg + 6*RASTERLINE_SIZE + 2) =
				*(btrg + 9*RASTERLINE_SIZE + 2) =
					(pattern & 0xFF);
				btrg += 3;
			}
			btrg += 9 * RASTERLINE_SIZE;
		}
		break; }
	  case 0x41: { // quadruple width, single height
		DWord *dwtrg = (DWord *)copy_block;
		for (i = 0; i < BLOCKHEIGHT; i++) {
			for (j = 0; j < RASTERLINE_SIZE; j++) {
				*dwtrg = conv_4byte_tab[*(src++)];
				dwtrg++;
			}
		}
		break; }
	  case 0x42: { // quadruple width, double height
		DWord *dwtrg = (DWord *)copy_block;
		for (i = 0; i < BLOCKHEIGHT; i++) {
			for (j = 0; j < RASTERLINE_SIZE; j++) {
				*dwtrg = *(dwtrg + RASTERLINE_SIZE) =
				  	conv_4byte_tab[*(src++)];
				dwtrg++;
			}
			dwtrg += RASTERLINE_SIZE;
		}
		break; }
	  case 0x43: { // quadruple width, triple height
		DWord *dwtrg = (DWord *)copy_block;
		for (i = 0; i < BLOCKHEIGHT; i++) {
			for (j = 0; j < RASTERLINE_SIZE; j++) {
				*dwtrg =
				  *(dwtrg +     RASTERLINE_SIZE) =
				  *(dwtrg + 2 * RASTERLINE_SIZE) =
				  	conv_4byte_tab[*(src++)];
				dwtrg++;
			}
			dwtrg += 2 * RASTERLINE_SIZE;
		}
		break; }
	  case 0x44: { // quadruple width, quadruple height
		DWord *dwtrg = (DWord *)copy_block;
		for (i = 0; i < BLOCKHEIGHT; i++) {
			for (j = 0; j < RASTERLINE_SIZE; j++) {
				*dwtrg =
				  *(dwtrg +     RASTERLINE_SIZE) =
				  *(dwtrg + 2 * RASTERLINE_SIZE) =
				  *(dwtrg + 3 * RASTERLINE_SIZE) =
				  	conv_4byte_tab[*(src++)];
				dwtrg++;
			}
			dwtrg += 3 * RASTERLINE_SIZE;
		}
		break; }
	  default:
		img = image1[0][0];
	} // switch

	if (!(e2video->vico1 & 0x02)) {
		if (block_number == e2video->divided_block) {
			// first half display on the bottom of the window
			XPutImage(dpy, win, e2gc,
				img, 0, 0, 0, (WINDOWHEIGHT -
				(e2video->vico2 % BLOCKHEIGHT)) * guiYSize,
				BLOCKWIDTH * guiXSize,
				(e2video->vico2 % BLOCKHEIGHT) * guiYSize);
			// second half display on the top of window
			XPutImage(dpy, win, e2gc,
				img, 0, (e2video->vico2 % YBLOCKS) * guiYSize,
				0, 0, BLOCKWIDTH * guiXSize, (BLOCKHEIGHT -
				   (e2video->vico2 % BLOCKHEIGHT)) * guiYSize);
		} else {
			XPutImage(dpy, win, e2gc,
				img, 0, 0, 0,
				((block_number * BLOCKHEIGHT + WINDOWHEIGHT -
				   e2video->vico2) % WINDOWHEIGHT) * guiYSize,
				BLOCKWIDTH * guiXSize, BLOCKHEIGHT * guiYSize);
		} // else
	} else {
		for (i = 0; i < YBLOCK_SIZE * guiXSize * guiYSize; i++)
			copy_block[i] = pOptions->inverse ? 0x00 : 0xff;
		// allways display an empty screen
			XPutImage(dpy, win, e2gc,
				image1[guiXSize-1][guiYSize-1], 0, 0, 0,
				block_number * BLOCKHEIGHT * guiXSize,
				BLOCKWIDTH * guiXSize, BLOCKHEIGHT * guiYSize);
	} // else
} // update_bw_block


SWord XAbstractGui::translate_to_ascii(XKeyEvent *pevent)
{
	KeySym		keysym;
	char		charString[2];
	int		count;

	// first check for control character
	count = XLookupString(pevent, charString, 2, &keysym, NULL);
//	fprintf(stderr, "%X\n", keysym); only needed for debugging
	if (pevent->state & ControlMask) {
		switch(keysym) {
			case XK_a: return 0x01;
			case XK_b: return 0x02;
			case XK_c: return 0x03;
			case XK_d: return 0x04;
			case XK_e: return 0x05;
			case XK_f: return 0x06;
			case XK_g: return 0x07;
			case XK_h: return 0x08;
			case XK_i: return 0x09;
			case XK_j: return 0x0a;
			case XK_k: return 0x0b;
			case XK_l: return 0x0c;
			case XK_m: return 0x0d;
			case XK_n: return 0x0e;
			case XK_o: return 0x0f;
			case XK_p: return 0x10;
			case XK_q: return 0x11;
			case XK_r: return 0x12;
			case XK_s: return 0x13;
			case XK_t: return 0x14;
			case XK_u: return 0x15;
			case XK_v: return 0x16;
			case XK_w: return 0x17;
			case XK_x: return 0x18;
			case XK_y: return 0x19;
			case XK_z: return 0x1a;
			case XK_8:         return 0x1b;
			case XK_ssharp:    return 0x1c;
			case XK_9:         return 0x1d;
			case XK_asciicircum:return 0x1e;
			case XK_minus:     return 0x1f;
			case XK_Return:    return 0x0d;
			case XK_Escape:    return 0x1b;
			case XK_Tab:       return 0x09;
			case XK_Delete:
			case XK_BackSpace: return 0x08;
			case XK_Break:
			case XK_Pause:     if (cpu) cpu->set_nmi(); return -1;

			case XK_Home:
			case XK_KP_7:	   return 0xb7;
			case XK_Up:
			case XK_KP_8:	   return 0xb8;
			case XK_Prior:
			case XK_KP_9:	   return 0xb9;
			case XK_Left:
			case XK_KP_4:	   return 0xb4;
			case XK_Begin:
			case XK_KP_5:	   return 0xb5;
			case XK_Right: 
			case XK_KP_6:	   return 0xb6;
			case XK_KP_1:	   return 0xb1;
			case XK_Down:
			case XK_KP_2:	   return 0xb2;
			case XK_Next:
			case XK_KP_3:	   return 0xb3;
			case XK_F11:	   return 0xfa; // PAT09: RIGHT MOST
			case XK_F12:	   return 0x91; // PAT09: LEFT  MOST
			default:           return -1;
		} // switch
	} // if
	if (pevent->state & ShiftMask) {
		switch(keysym) {
			case XK_F1:    return 0xca; // PAT09: F11
			case XK_F2:    return 0xcb; // PAT09: F12
			case XK_F3:    return 0xcc; // PAT09: F13
			case XK_F4:    return 0xcd; // PAT09: F14
			case XK_F5:    return 0xce; // PAT09: F15
			case XK_F6:    return 0xcf; // PAT09: F16
			case XK_F7:    return 0xd0; // PAT09: F17
			case XK_F8:    return 0xd1; // PAT09: F18
			case XK_F9:    return 0xd2; // PAT09: F19
			case XK_F10:   toggle_mouse_capture(); return -1;
			case XK_F11:   return 0xea; // PAT09: RIGHT MOST
			case XK_F12:   return 0x81; // PAT09: LEFT  MOST

			case XK_Break:
			case XK_Pause:     if (cpu) cpu->set_nmi(); return -1;
			case XK_Delete:
			case XK_BackSpace: return 0x7f; // PAT09: SHIFT DEL

			case XK_Home:
			case XK_KP_7:	   return 0xe7; // PAT09: SHIFT ARR UL
			case XK_Up:
			case XK_KP_8:	   return 0xe8; // PAT09: SHIFT CUR U
			case XK_Prior:
			case XK_KP_9:	   return 0xe9; // PAT09: SHIFT ARR R 
			case XK_Left:
			case XK_KP_4:	   return 0xe4; // PAT09: SHIFT CUR L
			case XK_Begin:
			case XK_KP_5:	   return 0xe5; // PAT09: SHIFT MODE
			case XK_Right: 
			case XK_KP_6:	   return 0xe6; // PAT09: SHIFT CUR R
			case XK_KP_1:	   return 0xe1; // PAT09: SHIFT ARR L 
			case XK_Down:
			case XK_KP_2:	   return 0xe2; // PAT09: SHIFT CUR D
			case XK_Next:
			case XK_KP_3:	   return 0xe3; // PAT09: SHIFT ARR DR
		} // switch
	} // if
	switch (keysym) {
		// main keyboard
		case XK_Return:    return 0x0d;
		case XK_Escape:    return 0x1b;
		case XK_Tab:       return 0x09;
		case XK_space:     return ' ';
		case XK_Delete:
		case XK_BackSpace: return 0x08;
		case XK_Break:
		case XK_Pause:     return 0x03;

		case XK_F1:        return 0xc0;
		case XK_F2:        return 0xc1;
		case XK_F3:        return 0xc2;
		case XK_F4:        return 0xc3;
		case XK_F5:        return 0xc4;
		case XK_F6:        return 0xc5;
		case XK_F7:        return 0xc6;
		case XK_F8:        return 0xc7;
		case XK_F9:        return 0xc8;
		case XK_F10:       return 0xc9;
		case XK_F11:       return 0xfa; // PAT09: RIGHT MOST
		case XK_F12:       return 0x91; // PAT09: LEFT  MOST
		
		case XK_Home:
		case XK_KP_7:	   return 0xf7;
		case XK_Up:
		case XK_KP_8:	   return 0xf8;
		case XK_Prior:
		case XK_KP_9:	   return 0xf9;
		case XK_Left:
		case XK_KP_4:	   return 0xf4;
		case XK_Begin:
		case XK_KP_5:	   return 0xf5;
		case XK_Right: 
		case XK_KP_6:	   return 0xf6;
		case XK_KP_1:	   return 0xf1;
		case XK_Down:
		case XK_KP_2:	   return 0xf2;
		case XK_Next:
		case XK_KP_3:	   return 0xf3;
		case XK_KP_Decimal:return ',';
		case XK_KP_Add:    return '+';
		case XK_KP_Subtract:return '-';
		case XK_KP_Enter:  return 0x0d;
	} // switch
	if (count == 1) {
		switch(charString[0]) {
			case '�': return '^';
			case '�': return '~';
			case '�': return '[';
			case '�': return '\\';
			case '�': return ']';
			case '�': return '{';
			case '�': return '|';
			case '�': return '}';
			default: return charString[0];
		} // switch
	}
	return -1;
} // translate_to_ascii

int XAbstractGui::convert_buttonmask(int newButtonMask)
{
	int mask;

	mask = 0;
       // convert to platform independant flags
	if (newButtonMask & Button1Mask)
		mask |= L_MB;
	if (newButtonMask & Button2Mask)
		mask |= M_MB;
	if (newButtonMask & Button3Mask)
		mask |= R_MB;
	if (newButtonMask & ShiftMask)
		mask |= SHIFT_KEY;
	if (newButtonMask & ControlMask)
		mask |= CONTROL_KEY;
	return mask;
}

void XAbstractGui::set_bell(int percent)
{
	XBell(getDisplay(), percent);
} // set_bell

// return: -1 = failure
int XAbstractGui::popup_help(void)
{
#ifdef WITH_HELP
	pid_t child_pid;
	int status;
	int success;
	char helpfile[PATH_MAX];
	const char *args[3];

	// if no environment variable for browser return with error
	if (strlen(pOptions->www_browser) == 0)
		return -1;
	if (strlen(pOptions->doc_dir) != 0)
		strcpy(helpfile, pOptions->doc_dir);
	else
		strcpy(helpfile, "/usr/local/doc/flexemu");
	if (helpfile[strlen(helpfile)-1] != '/')
		strcat(helpfile, "/");
	strcat(helpfile, "flexemu.htm");
	args[0] = pOptions->www_browser;
	args[1] = helpfile;
	args[2] = NULL;
	if ((child_pid = fork()) == 0) {
		// try to start WWW Browser
		success = execvp(args[0], const_cast<char **>(args));
		// if it fails exit with errorcode
		exit(255);
	} else {
		sleep(1);
		success = waitpid(child_pid, &status, WNOHANG);
		if (success == -1 ||
		   (success > 0 &&
		   WIFEXITED(status) != 0 &&
		   WEXITSTATUS(status) == 255)) // check for errorcode
			return -1;
	} // else
#endif
	return 0;
} // popup_help

void XAbstractGui::create_message_dialog(Widget parent)
{
} // create_message_dialog

XAbstractGui::XAbstractGui(
	Mc6809*    x_cpu,
	Memory*    x_memory,
	Scheduler* x_sched,
	Inout*     x_io,
	E2video*   x_video,
	struct sGuiOptions *pOptions) :
		AbstractGui(x_cpu, x_memory, x_sched, x_io, x_video, pOptions),
		cursor(None), cursor_type(FLX_DEFAULT_CURSOR)
{
}

XAbstractGui::~XAbstractGui()
{
}

Visual *XAbstractGui::GetBestVisual(Display *dpy, int *vClass, int *depth)
{
	int default_depth;
	XVisualInfo vTemplate;
	XVisualInfo *visualList = NULL;
	Visual *visual = NULL;
	int nrOfVisuals;

	vTemplate.screen = DefaultScreen(dpy);
	default_depth = DefaultDepth(dpy, vTemplate.screen);
	switch (default_depth) {
	  case 8:
		*vClass = vTemplate.c_class  = PseudoColor;
		visualList = XGetVisualInfo(dpy,
			VisualScreenMask | VisualClassMask,
			&vTemplate, &nrOfVisuals);
		if (nrOfVisuals > 0) {
			*depth = visualList[0].depth;
			visual = visualList[0].visual;
		} else {
			*vClass = vTemplate.c_class = TrueColor;
			visualList = XGetVisualInfo(dpy,
				VisualScreenMask | VisualClassMask,
				&vTemplate, &nrOfVisuals);
			if (nrOfVisuals > 0) {
				*depth = visualList[0].depth;
				visual = visualList[0].visual;
			} else {
				*vClass = vTemplate.c_class  = GrayScale;
				visualList = XGetVisualInfo(dpy,
					VisualScreenMask | VisualClassMask,
					&vTemplate, &nrOfVisuals);
				if (nrOfVisuals > 0) {
					*depth = visualList[0].depth;
					visual = visualList[0].visual;
				}
			}
		}
		break;
	  case 16:	
	  case 24:	
	  case 32:	
		*vClass = vTemplate.c_class  = TrueColor;
		visualList = XGetVisualInfo(dpy,
			VisualScreenMask | VisualClassMask,
			&vTemplate, &nrOfVisuals);
		if (nrOfVisuals > 0) {
			*depth = visualList[0].depth;
			visual = visualList[0].visual;
		}
		break;
	}
	XFree(visualList);
	return visual;
}

int XAbstractGui::SetColors(Display *dpy)
{
	int screen;
	XcmsColor xcolor;
	XcmsColor exact;
	int i, scale;
	int rw_color;	/* is it a read/write color ?*/
	int visualClass;
	unsigned long pixels[1 << COLOR_PLANES];

	screen = DefaultScreen(dpy);
	visual = GetBestVisual(dpy, &visualClass, &depth);
	if (!visual) {
		fprintf(stderr, "Unable to find an appropriate visual\n");
		return 0;
	}
	rw_color = (visualClass == DirectColor || visualClass == PseudoColor ||
		visualClass == GrayScale);
	cmap = DefaultColormap(dpy, screen);
	if (rw_color) {
		/* on visuals with read/write color cells 
		   first allocate them */
		if (!XAllocColorCells(dpy, cmap, False, NULL, 0, pixels, 
			1 << COLOR_PLANES)) {
			/* try again with a own colormap */
			cmap = XCreateColormap(dpy,
				RootWindow(dpy, screen),
				visual, AllocNone);
			own_cmap = 1;
			if (!XAllocColorCells(dpy, cmap, False, NULL, 0, 
				pixels, 1 << COLOR_PLANES)) {
				fprintf(stderr, "Unable to allocate %d colors\n",
					1 << COLOR_PLANES);
				return 0; /* failed even with own colormap */
			}
		}
	}

	if (!XcmsLookupColor(dpy, cmap, color, &xcolor, &exact, XcmsRGBiFormat))
	{
		exact.spec.RGBi.blue  = 1.0;
		exact.spec.RGBi.red   = 1.0;
		exact.spec.RGBi.green = 1.0;
	}
	xcolor.format = XcmsRGBiFormat;
	for (i = 0; i < (1 << COLOR_PLANES); i++) {
		if (use_colors) {
			// DEPENDANCIES:
			// the color plane masks used here depend on
			// the same masks used in CopyToZPixmap
			scale  = i & 0x20 ? 2 : 0; scale |= i & 0x04 ? 1 : 0;
			xcolor.spec.RGBi.green = 1.0 * scale / 3;
			scale  = i & 0x10 ? 2 : 0; scale |= i & 0x02 ? 1 : 0;
			xcolor.spec.RGBi.red   = 1.0 * scale / 3;
			scale  = i & 0x08 ? 2 : 0; scale |= i & 0x01 ? 1 : 0;
			xcolor.spec.RGBi.blue  = 1.0 * scale / 3;
		} else {
			xcolor.spec.RGBi.blue  =
				exact.spec.RGBi.blue  * i / (1 << COLOR_PLANES);
			xcolor.spec.RGBi.red   =
				exact.spec.RGBi.red   * i / (1 << COLOR_PLANES);
			xcolor.spec.RGBi.green =
				exact.spec.RGBi.green * i / (1 << COLOR_PLANES);
		}
		if (rw_color) {
			xcolor.pixel = pixels[i];
			if (XcmsStoreColor(dpy, cmap,
				&xcolor) == XcmsFailure) {
				fprintf(stderr, "Error storing a color\n");
				return 0;
			}
		} else {
			if (XcmsAllocColor(dpy, cmap,
				&xcolor, XcmsRGBiFormat) == XcmsFailure) {
				fprintf(stderr, "Error allocating a color\n");
				return 0;
			}
		}
		pen[i] = xcolor.pixel;
	}
	//gcVal.plane_mask = 0xFFFFFFFF;

	return 1;
}


#endif // #ifdef HAVE_X11
