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

#ifndef JENN_PROJECTION_H
#define JENN_PROJECTION_H

#include "definitions.h"
#include "animation.h"
#include "drawing.h"
#include "trail.h"
#include "linalg.h"

//stereo params
#define TWIST_ANGLE 0.06

namespace Projection
{

const Logging::Logger logger("projn", Logging::INFO);

//projection class
class Projector
{
    Trails::Trail *trail;
    int w, W, h;                //width, adjusted width, height
    float w_factor, h_factor;   //width and height scaling factors
    float x_center, y_center;   //center for panning
    bool in_stereo;
    bool trailing, trail_paused;
    bool high_quality;
    bool high_contrast, reverse_colors, motion_blur;
    float aperture_size, depth; //for depth-of-field
    Mat tilt;                   //tilted version
    Mat temp1, temp2, temp3;

    bool _update_needed;
    bool _update_accum;
    void _update();
public:
    bool paused;
    void update (bool update_accum = true);
    void display (char* output=NULL);
    void reset ();

public:
    Projector ()
        : trail(NULL),
          paused(false)
    { reset(); set_drawing(false); }

    //view control
    void init_size (int _w, int _h) //no glutPostRedisplay yet
    { w=_w; h=_h;  _update_needed = _update_accum = true; }
    void reshape (int _w, int _h) { w=_w; h=_h; update(); }
    void toggle_stereo () { in_stereo = not in_stereo; update(); }
    void set_stereo (bool new_val) { in_stereo = new_val; update(); }
    void zoom (float factor) { animator->zoom(factor); update(); }
    int select (int X, int Y);
    bool get_quality () { return high_quality; }
    void toggle_quality ();
    void toggle_trail (Trails::Style style=Trails::RIBBON);
    void toggle_trailing ();
    void pause_trail () { trail_paused = not trail_paused; }
    void toggle_wireframe ()
    {
        drawing->toggle_wireframe();
        if (trail) trail->toggle_wireframe();
    }

    //panning
    void pan_center () { x_center = 0.0f; y_center = 0.0f; update(); }
    void pan_left (float d=2)
    { x_center -= d * animator->vis_rad * w_factor; update(); }
    void pan_right (float d=2)
    { x_center += d * animator->vis_rad * w_factor; update(); }
    void pan_up (float d=2)
    { y_center += d * animator->vis_rad * h_factor; update(); }
    void pan_down (float d=2)
    { y_center -= d * animator->vis_rad * h_factor; update(); }

    //conversions: screen -> real space
    float convert_x (float X)
        { return x_center - animator->vis_rad * w_factor * (1.0-(2.0*X)/W); }
    float convert_y (float Y)
        { return y_center + animator->vis_rad * h_factor * (1.0-(2.0*Y)/h); }

    //exposure
    void toggle_contrast () { high_contrast = not high_contrast; update(); }
    void toggle_reversed () { reverse_colors = not reverse_colors; update(); }
    void toggle_blur ();

    //depth-blur
    void scale_aperture (float scale) { aperture_size *= scale; update(false); }
    void move_nearer  () { depth = max(depth-0.15f, -1.5f); update(false); }
    void move_farther () { depth = min(depth+0.15f, +1.5f); update(false); }
    void _bound_image (float x_shift = 0, float y_shift = 0);
    void _draw_buffer ();
    void _show_buffer (char* output=NULL);
    void _draw ();
    void _set_tilt (float theta, float phi);
    void _set_rand_tilt ();
    void _set_unif_tilt (int n, int N);

#ifdef CAPTURE
    //high-resolution capture
private:
    bool in_color;
    void _capture_little (char* image);
public:
    void set_color (bool ic) { in_color = ic; }
    void capture (unsigned Nwide, unsigned Nhigh);
#endif

    void set_drawing (bool updating=true);
};

}

//global instance
extern Projection::Projector *projector;

#endif

