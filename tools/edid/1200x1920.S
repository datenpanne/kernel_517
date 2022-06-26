/*
   1600x1200.S: EDID data set for standard 1600x1200 60 Hz monitor

   Copyright (C) 2013 Carsten Emde <C.Emde@osadl.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
*/

/* EDID */
#define VERSION 1
#define REVISION 3

/* Display */
#define CLOCK 161000 /* kHz */
#define XPIX 1200
#define YPIX 1920
#define XY_RATIO XY_RATIO_16_10
#define XBLANK 160
#define YBLANK 55
#define XOFFSET 48
#define XPULSE 32
#define YOFFSET 3
#define YPULSE 10
#define DPI 226
#define VFREQ 60 /* Hz */
#define TIMING_NAME "Linux WSXGA"
/* No ESTABLISHED_TIMINGx_BITS */
#define HSYNC_POL 1
#define VSYNC_POL 1

#include "edid.S"
