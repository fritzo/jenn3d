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

#ifndef JENN_DRAWING_H
#define JENN_DRAWING_H

#include "definitions.h"
#include "go_game.h"
#include "linalg.h" //for hopf_phase
#include "aligned_vect.h"

//[ depth sorting graph drawing ]----------
namespace Drawings
{

const Logging::Logger logger("draw", Logging::INFO);

#define COLOR_BG      1.0, 1.0,  1.0
#define COLOR_FG      0.0, 0.0,  0.0
//#define COLOR_BG      0.9, 0.9,  0.9
//#define COLOR_BG      1.0, 1.0,  1.0
//#define COLOR_BG      0.9, 0.8,  0.5
//#define COLOR_BG      0.6, 0.4, 0.8
//#define COLOR_BG      0.8, 0.9,  0.9
//#define COLOR_BG      0.5, 0.3, 0.4
#define POLY_SIDES   60
#define LINE_SIDES   60
#define FACE_SIDES   32
#define SPH_RHO   12
#define SPH_THETA 48
#define MAX_DEG 20

class DepthCmp
{
private:
    const int ord;
    float * const values;
public:
    DepthCmp (int _ord) : ord(_ord), values(new float[_ord])
    { logger.debug() << "init'ing DeptCmp of size " << ord |0; }
    //~DepthCmp () { delete[] values; } //this seems to crash
    void update(const nonstd::aligned_vect<Vect>& centers) const
    { for (int i=0; i<ord; ++i) values[i] = centers[i][2]; }
    bool operator () (int v1, int v2) const
    { return values[v1] < values[v2]; }
};

class Drawing
{
    //data
    GoGame::GO go;
    ToddCoxeter::Graph &graph;
    const int ord, deg, ord_f;
    float w_bound0, w_bound1, h_bound0, h_bound1;
    const DepthCmp cmp, cmp_f;
    std::vector<int> sorted;         //depth-sorted vertices
    std::vector<int> sorted_f;       //depth-sorted faces
    typedef nonstd::aligned_vect<Vect> vvector;
    vvector vertices;                //vertex locations
    vvector centers;                 //centers
    std::vector<float> radii0;       //unscaled radii
    std::vector<float> scales;       //vertex scales
    std::vector<float> radii;        //radii
    std::vector<complex> phases;     //blinkin' hopf phases
    typedef std::vector<int> Face;
    const std::vector<Face> &faces;  //faces
    vvector vertices_f, normals;     //face centers & normal vectors
    vvector centers_f;               //projected face centers
    float rad0, sph_rad0, sph_rad;   //standard radius sizes
    float tube_rad, tube_factor;     //std tube sizes
    float coating;                   //extra coating for exporting
    Mat   project;                   //orthogonal (pre-)projection matrix

    float poly0[POLY_SIDES][2];      //original polygon
    float poly1[POLY_SIDES][3];      //temporarily transformed polygon
    float poly2[POLY_SIDES][3];      //temporarily transformed polygon
    float shade1[POLY_SIDES];
    float shade2[POLY_SIDES];

    float lines[LINE_SIDES+2][3];    //lines
    float normal[LINE_SIDES+2][3];   //normal directions
    float bord1[LINE_SIDES+2][3];    //border lines
    float bord2[LINE_SIDES+2][3];    //border lines
    float width[LINE_SIDES+2];       //line widths

    float sphere[SPH_RHO+1][SPH_THETA][3];

    Vect midpoint[MAX_DEG];          //temporary midpoint vector
    Vect contact[MAX_DEG];           //temporary contact-point vector
    Vect farpoint[MAX_DEG];          //temporary far-point vector
    float w_val[MAX_DEG];

    std::vector<std::pair<float,int> > ordered_lines;

    //drawing parameters
    float scale, q_scale;
    bool _grid_on;
    bool _drawing_verts, _drawing_edges, _drawing_faces;
    bool _fancy, _hazy, _wireframe, _curved, _high_quality;
    bool _clipping;
    bool _update_needed;
public:
    int get_params ();
    void set_params (int params);
    void update () { _update_needed = true; }
    void _update();
    inline void toggle_grid () { _grid_on = not _grid_on; }
    inline void toggle_verts () { _drawing_verts = not _drawing_verts; }
    inline void toggle_edges () { _drawing_edges = not _drawing_edges; }
    inline void toggle_faces () { _drawing_faces = not _drawing_faces; }
    void toggle_fancy ();
    void toggle_hazy ();
    void toggle_wireframe ();
    void toggle_curved ();
    void set_quality (bool quality);
    void set_scale (float scale);
    float get_tube_rad () { return tube_rad; }
    void set_tube_rad (float rad);
    float get_coating () { return coating; }
    void set_coating (float thickness) { coating = thickness; }
    void set_bounds (float w0, float w1, float h0, float h1);
    void set_clipping (bool clipping) { _clipping = clipping; }

    //ctors & dtors
    //Drawing (GoGame::GO *_go);
    Drawing (ToddCoxeter::Graph* g);

    //wrappers for go board
    void play (int v, int s) { go.play(v,s); }
    void back () { go.back(); }
    void forward () { go.forward(); }

    //methods
    float get_radius ();
    void reproject (Mat& theta);
    void display ();    //using current projection
    void export_stl (); //using current projection
    void export_graph ();
    int select (float x,float y);
private:
    int _num_segments (float w, float dist);
    int _secant_stride (float w, float rad);
    int _num_subdivs (float w, int Nfaces);
    void _draw_bulb     (float* center, float radius);
    void _draw_sphere   (float* center, float radius, int v);
    void _export_sphere (float* center, float radius, int v);
    void _draw_arc      (Vect& begin, Vect& end, float w);
    void _draw_arc2     (Vect& begin, Vect& end, float w);
    void _draw_arc_wide (Vect& begin, Vect& end, float w);
    void _draw_tube     (Vect& begin, Vect& end,
                         float r0, float r1, float w, int v0, int v1);
    void _export_tube   (Vect& begin, Vect& end,
                         float r0, float r1, float w, int v0, int v1);
    void _draw_face (int f);

    void display_vertex (int v);
    void export_vertex (int v);
    void sort ();
    inline void update_vertex (int v);
    inline void update_face (int f);
};

}

//global instance
extern Drawings::Drawing *drawing;

#endif

