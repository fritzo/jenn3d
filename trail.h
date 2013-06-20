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

#ifndef JENN_TRAIL_H
#define JENN_TRAIL_H

#include "definitions.h"
#include "linalg.h"
#include <vector>

namespace Trails
{

const Logging::Logger logger("trail", Logging::DEBUG);

enum Style { LINE, RIBBON, TUBE };

class Trail
{
    typedef Vect Point;
    typedef std::vector<Point> Points;
    typedef float Time;
    typedef std::vector<Time> Times;

    Point m_origin;
    std::vector<Points> m_points, m_projected;
    std::vector<Times> m_times;
    const Style style;
    bool _wireframe;
public:
    Trail (Style s=TUBE)
        : m_points(1), m_projected(1), m_times(1),
          style(s), _wireframe(false)
    { m_origin[0]=0; m_origin[1]=0; m_origin[2]=0; m_origin[3]=-1; }

    void add_point (const Mat& theta, float time);
    void new_trail ();
    void display (const Mat& theta, float time=0.0f);

    void toggle_wireframe () { _wireframe = not _wireframe; }

private:
    void _draw_line     (int n, const Mat& theta, float time);
    void _draw_ribbon   (int n, const Mat& theta, float time);
    void _draw_tube     (int n, const Mat& theta, float time);
};

}

#endif

