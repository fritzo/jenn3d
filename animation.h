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

#ifndef JENN_ANIMATION_H
#define JENN_ANIMATION_H

#include "definitions.h"
#include "linalg.h"

namespace Animation
{

const Logging::Logger logger("anime", Logging::INFO);

class Animate
{
public:
    double time;                    //elapsed system time
    float vis_rad0, vis_rad;        //visual radius
    int drag_channels;              //bitfield of active drag channels
    float drag_x, drag_y;           //current dragging position
    float drag_dx, drag_dy;         //dragging differentials
    float noise0, noise1;           //process noise
    float decay0, decay1, decay2, decay3; //drift damping
    float strength0, strength1;     //control strength
    float strengthC;                //centering strength
    Mat theta, theta0,              //rotational positions
        omega, omega0,              //rotational velocity
        alpha0, alpha1,             //rotational acceleration
        delta,                      //rotational displacement
        temp1, temp2;               //working matrices

    Animate ();

    Mat& twist_theta (float twist); //for stereo

    //motion params
    bool stopped;                   //drift control
    bool centered;                  //centering control
    bool flying,drifting;           //translation control
    float speed;                    //time scale
    void toggle_stopped ();
    void toggle_centered () { centered = not centered; }
    void toggle_flying ();
    void toggle_drifting () { drifting = not drifting; }
    void accel_fwd(float dv=0.1f);
    void slow_down () { speed *= 0.5f; }
    void speed_up  () { speed *= 2.0f; }

    void reset (bool state_only=false);
    void reset_pos ();
    void reset_veloc ();
    void reset_accel ();
    void set_radius0 (float rad);
    void set_radius (float rad);
    void zoom (float factor) { set_radius(vis_rad * factor); }
    void set_center ();
    void invert ();

    //general control
    int _channel_shift;
    void shift_channels () { _channel_shift = (1+_channel_shift) % 3; }
    void set_drag (float x, float y);
    void rot_drag (float x, float y);
    void set_trans_force (float x, float y, float z);
    void set_rot_force   (float x, float y, float z);
    void drift (float dt);
};

}

//global instance
extern Animation::Animate* animator;

#endif

