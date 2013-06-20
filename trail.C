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

#include "trail.h"
#ifdef CYGWIN_HACKS
    #define GLUT_STATIC
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#define LINE_RATE       36.0f
#define RIBBON_RATE     24.0f
#define TUBE_RATE       8.0f
#define COLOR_TRAIL1    1.0f, 0.0f, 0.0f
#define COLOR_TRAIL2    0.0f, 0.0f, 0.0f
#define COLOR_TRAIL3    1.0f, 1.0f, 1.1f

#define SWAY 0.006f
#define TUBE_DIAM 0.005f
#define SIDES 4

//================ colors ================
float _ct1[3] = {COLOR_TRAIL1};
float _ct2[3] = {COLOR_TRAIL2};
float _ct3[3] = {COLOR_TRAIL3};
float _result_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
typedef float* Color;
inline Color get_color (float phi)
{
    const float offset = 2.0f * M_PI / 3.0f;
    float s1 = (1.0f - cosf(phi         )) / 3.0f;
    float s2 = (1.0f - cosf(phi - offset)) / 3.0f;
    float s3 = (1.0f - cosf(phi + offset)) / 3.0f;
    for (int i=0; i<3; ++i) {
        _result_color[i] = s1 * _ct1[i] + s2 * _ct2[i] + s3 * _ct3[i];
    }
    _result_color[3] = 1.0f - sqr(0.5f * (1.0f + cosf(phi)));
    return _result_color;
}

namespace Trails
{
//WARNING: this must match fun in drawing.C
inline float clamp_depth (float z) { return (2.0f/M_PI) * atanf(0.5f*z); }

void Trail::add_point (const Mat& theta, float time)
{
    Mat itheta; mat_inverse(theta,itheta);
    Vect p;
    const float rho = 4.0f * M_PI / (2*SIDES+1);
    switch (style) {
        case LINE: {
            vect_mult(itheta, m_origin, p);
        } break;
        case RIBBON: {
            static bool side=false; side = not side;
            Mat rot;                mat_rot(0,3, side ? SWAY : -SWAY, rot);
            Vect q;                 vect_mult(rot, m_origin, q);
            vect_mult(itheta, q, p);
        } break;
        case TUBE: {
            static int side=0;  side = (side+1)%(2*SIDES+1);
            Mat rot1;           mat_rot(0,3, TUBE_DIAM, rot1);
            Mat rot2;           mat_rot(0,1, rho * side, rot2);
            Mat rot3;           mat_conj(rot2, rot1, rot3);
            Vect q;             vect_mult(rot3, m_origin, q);
            vect_mult(itheta, q, p);
        } break;
    }
    m_points.back().push_back(p);
    m_times.back().push_back(time);
}
void Trail::new_trail ()
{
    m_points   .push_back(Points(0));
    m_projected.push_back(Points(0));
    m_times    .push_back(Times(0));
}

//drawing
GLenum FILL = GL_FILL;
void Trail::display (const Mat& theta, float time)
{

    if (_wireframe) {
        glLineWidth(1.0f);
        FILL = GL_LINE;
    } else {
        FILL = GL_FILL;
    }

    switch (style) {
        case LINE:
            for (unsigned n=0; n<m_points.size(); ++n) 
                _draw_line(n, theta, time);
            break;
        case RIBBON:
            for (unsigned n=0; n<m_points.size(); ++n) 
                _draw_ribbon(n, theta, time);
            break;
        case TUBE:
            for (unsigned n=0; n<m_points.size(); ++n) 
                _draw_tube(n, theta, time);
            break;
    }
}
void Trail::_draw_line (int n, const Mat& theta, float time)
{
    Points &points    = m_points[n];
    Points &projected = m_projected[n];
    Times  &times     = m_times[n];

    unsigned N = points.size();
    if (N<3) return;
    projected.resize(N);

    //construct points
    for (unsigned n=0; n<N; ++n) {
        Vect p;
        vect_mult(theta, points[n], p);
        float scale = fabs(1.0f / (1.0f + p[3]));
        p[0] *= scale;
        p[1] *= scale;
        p[2] *= scale;
        p[2] = clamp_depth(p[2]);
        //p[3] = scale;
        projected[n] = p;
    }

    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2.0f);

    glBegin(GL_LINE_STRIP);
    for (unsigned n=0; n<N-2; ++n) { //don't draw the final point
        glColor4fv(get_color(LINE_RATE * (times[n] - time)));
        //const Vect& p = projected[n];
        //glVertex3fv(p.data);
        glVertex3fv(projected[n].data);
    }
    glEnd();
}
void Trail::_draw_ribbon (int n, const Mat& theta, float time)
{
    Points &points    = m_points[n];
    Points &projected = m_projected[n];
    Times  &times     = m_times[n];

    unsigned N = points.size();
    if (N<3) return;
    projected.resize(N);

    //construct points
    for (unsigned n=0; n<N; ++n) {
        Vect p;
        vect_mult(theta, points[n], p);
        float scale = fabs(1.0f / (1.0f + p[3]));
        p[0] *= scale;
        p[1] *= scale;
        p[2] *= scale;
        p[2] = clamp_depth(p[2]);
        //p[3] = scale;
        projected[n] = p;
    }

    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, FILL);

    glBegin(GL_TRIANGLE_STRIP);
    for (unsigned n=0; n<N-2; ++n) { //don't draw the final point
        glColor4fv(get_color(RIBBON_RATE * (times[n] - time)));
        //const Vect& p = projected[n];
        //glVertex3fv(p.data);
        glVertex3fv(projected[n].data);
    }
    glEnd();
}
void Trail::_draw_tube (int n, const Mat& theta, float time)
{
    Points &points    = m_points[n];
    Points &projected = m_projected[n];
    Times  &times     = m_times[n];

    unsigned N = points.size();
    if (N<3+SIDES) return;
    projected.resize(N);

    //construct points
    for (unsigned n=0; n<N; ++n) {
        Vect p;
        vect_mult(theta, points[n], p);
        float scale = fabs(1.0f / (1.0f + p[3]));
        p[0] *= scale;
        p[1] *= scale;
        p[2] *= scale;
        p[2] = clamp_depth(p[2]);
        //p[3] = scale;
        projected[n] = p;
    }

    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glPolygonMode(GL_FRONT, FILL);
    glEnable(GL_CULL_FACE);

    glBegin(GL_TRIANGLE_STRIP);
    for (unsigned n=0; n<N-2-SIDES; ++n) { //don't draw the final point
        glColor4fv(get_color(TUBE_RATE * (times[n+SIDES] - time)));
        glVertex3fv(projected[n+SIDES].data);
        glColor4fv(get_color(TUBE_RATE * (times[n] - time)));
        glVertex3fv(projected[n].data);
    }
    glEnd();
}

}

