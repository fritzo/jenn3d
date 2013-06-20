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

#include "animation.h"

#define INIT_SPEED 0.01f
#define SPEED_FACTOR 1.5f
#define ACCEL_FACTOR 4.0f
#define MIN_RADIUS  0.1f
#define MAX_RADIUS  100.0f

//global instance
Animation::Animate *animator = NULL;

namespace Animation
{

//[ animation class ]----------
Animate::Animate ()
{
    vis_rad0 = 1.0f;
    noise0 = 0.005f;
    noise1 = 0.0005f;
    decay1 = 0.5f;
    decay2 = 10.0f;
    decay3 = 0.3f;
    strength0 = 2.0f; //mouse control strength
    strength1 = 0.1f; //spaceball control strength
    strengthC = 4.0f;

    reset();

    _channel_shift = 0;
    drag_channels = 0;
}
Mat& Animate::twist_theta (float twist)
{
    mat_rot (2,0,twist,temp1);
    mat_mult (temp1, theta, temp2);
    return temp2;
}
void Animate::toggle_stopped ()
{
    if (stopped) mat_copy(omega, omega0);
    else         mat_zero(omega0);
    stopped = not stopped;
}
void Animate::accel_fwd (float dv)
{
    omega[2][3] += dv;
    omega[3][2] -= dv;
}
void Animate::toggle_flying ()
{
    flying = not flying;
    drifting = flying;
    if (flying) accel_fwd();
}
void Animate::reset_pos () { mat_identity(theta);  mat_identity(theta0); }
void Animate::reset_veloc () { mat_zero(omega); mat_zero(omega0); }
void Animate::reset_accel () { mat_zero(alpha0); mat_zero(alpha1); }
void Animate::reset (bool state_only)
{
    time = 0.0;
    reset_pos();
    reset_veloc();
    reset_accel();
    speed = 1.0f;

    if (not state_only) {
        stopped = true;
        centered = false;
        flying = false;
        drifting = false;
    }

    if (flying) accel_fwd();
}
void Animate::set_radius0 (float rad) { vis_rad0 = rad; set_radius(vis_rad0); }
void Animate::set_radius (float rad)
{
    vis_rad = rad;
    if (vis_rad < MIN_RADIUS) vis_rad = MIN_RADIUS;
    if (vis_rad > MAX_RADIUS) vis_rad = MAX_RADIUS;
}
void Animate::set_center () { mat_copy(theta, theta0); }
void Animate::invert () { mat_iscale(theta0, -1.0f); }
void Animate::set_drag (float x, float y)
{
    drag_x = x / vis_rad;
    drag_y = y / vis_rad;
    if (flying) reset_accel();
}
void Animate::rot_drag (float x, float y)
{
    //calculate displacements
    float dx = x - vis_rad * drag_x;
    float dy = y - vis_rad * drag_y;
    set_drag(x,y);

    //calculate scales
    float rot_scale   = 1.0 / vis_rad;
    float trans_scale = 2.0 / (1.0 + sqr(vis_rad));

    int channels = (drag_channels << _channel_shift)
                 | (drag_channels >> (3-_channel_shift));

    if (not flying) {
        //rotate (x,y) <--> z
        if (channels & 1) {  //drag channel 0 is active
            alpha0[0][2] += dx * rot_scale;
            alpha0[2][0] -= dx * rot_scale;

            alpha0[1][2] += dy * rot_scale;
            alpha0[2][1] -= dy * rot_scale;
        }

        //rotate (z <--> w), (x <--> y)
        if (channels & 2) {  //drag channel 1 is active
            alpha0[0][1] += dx * rot_scale;
            alpha0[1][0] -= dx * rot_scale;

            alpha0[3][2] += dy * trans_scale;
            alpha0[2][3] -= dy * trans_scale;
        }

        //rotate (x,y) <--> w
        if (channels & 4) {  //drag channel 2 is active
            alpha0[0][3] += dx * trans_scale;
            alpha0[3][0] -= dx * trans_scale;

            alpha0[1][3] += dy * trans_scale;
            alpha0[3][1] -= dy * trans_scale;
        }
    } else {
        //rotate (x,y) <--> z
        if (channels & 1) {  //drag channel 0 is active
            alpha0[0][2] += dx * rot_scale;
            alpha0[2][0] -= dx * rot_scale;

            alpha0[1][2] += dy * rot_scale;
            alpha0[2][1] -= dy * rot_scale;
        }

        //rotate (x,y) <--> w
        if (channels & 2) {  //drag channel 2 is active
            alpha0[0][3] += dx * trans_scale;
            alpha0[3][0] -= dx * trans_scale;

            alpha0[1][3] += dy * trans_scale;
            alpha0[3][1] -= dy * trans_scale;
        }

        //rotate (z <--> w), (x <--> y)
        if (channels & 4) {  //drag channel 1 is active
            alpha0[0][1] -= dx * rot_scale;
            alpha0[1][0] += dx * rot_scale;

            alpha0[3][2] -= dy * trans_scale;
            alpha0[2][3] += dy * trans_scale;
        }
    }
}
void Animate::set_trans_force (float x, float y, float z)
{
    alpha1[0][3] =  x;
    alpha1[3][0] = -x;

    alpha1[1][3] =  y;
    alpha1[3][1] = -y;

    alpha1[2][3] =  z;
    alpha1[3][2] = -z;
}
void Animate::set_rot_force (float x, float y, float z)
{
    alpha1[1][2] =  x;
    alpha1[2][1] = -x;

    alpha1[2][0] =  y;
    alpha1[0][2] = -y;

    alpha1[0][1] =  z;
    alpha1[1][0] = -z;
}

void Animate::drift (float dt)
{
    //adjust time scale
    dt *= speed;
    time += dt;

    if (not (drifting or flying)) {
        //add noise
        mat_isub(omega,omega0); //conjugate in
        float noise = sqrt(dt) * (stopped ? noise1 : noise0);
        rand_asym_mat(temp1, noise);
        mat_iadd(omega, temp1);

        //damp to slow down
        float decay = (stopped ? decay1 : decay0);
        float factor = exp(-dt * decay);
        mat_iscale(omega, factor);
        mat_iadd(omega, omega0); //conjugate out

    }

    //accel towards center
    if (centered and not flying) {
        mat_trans(theta, temp1);
        mat_mult(theta0, temp1, temp2);
        make_asym(temp2);
        mat_iscale(temp2, strengthC * dt);
        mat_iadd(omega, temp2);
    }

    //spaceball control
    mat_copy(alpha1, temp1);
    mat_iscale(temp1, strength1 * dt);
    mat_iadd(omega, temp1);

    //mouse control
    if (drag_channels) {
        if (not flying) {
            float strength = strength0 * decay2;
            mat_iscale(alpha0, strength);
            mat_iadd(omega, alpha0);
            mat_zero(alpha0);

            float factor = exp(-dt * decay2);
            mat_iscale(omega, factor);
        } else {
            //make rotational accel proportional to speed
            float speed = SPEED_FACTOR * sqrt(fabs(omega[2][3]) * vis_rad);
            mat_copy(alpha0, delta);
            delta[0][2] *= speed;
            delta[2][0] *= speed;
            delta[1][2] *= speed;
            delta[2][1] *= speed;

            //update velocity
            mat_iscale(delta, ACCEL_FACTOR);
            mat_iadd(omega, delta);

            float factor = exp(-dt * decay3);
            //damp scale all but z-w component, i.e., [2,3] and [3,2]
            for (int i=0; i<4; ++i) {
                for (int j=0; j<4; ++j) {
                    if (i*j != 6) omega[i][j] *= factor;
                }
            }
        }
    }

    //rotate theta by omega dt
    mat_mult(omega, theta, delta);
    mat_iscale(delta, dt);
    mat_iadd(theta, delta);
    make_ortho(theta);
}

}

