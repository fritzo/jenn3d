/*
This file is part of Jenn.
Copyright 2001-2007 Fritz Obermeyer.

Jenn is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Jenn is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Jenn; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef JENN_MAIN_H
#define JENN_MAIN_H

#include "definitions.h"
#include "drawing.h"
#include "animation.h"
#include "projection.h"
#include "polytopes.h"

const float ZOOM_OUT = pow(3.0, 0.2);
const float ZOOM_IN = 1.0f / ZOOM_OUT;

//functions needed by menu system
void finish_buffer ();
void update_title ();
void toggle_fullscreen ();
void drift ();
void end_pause ();
void beg_pause (bool redraw = true);
void toggle_pause ();

#endif

