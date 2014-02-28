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

#include "drawing.h"

#ifdef CYGWIN_HACKS
    #define GLUT_STATIC
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <cmath>
#include <cstring> //for memcpy
#include <utility>
#include <algorithm>
#include <fstream>

#ifndef INFINITY
#define INFINITY 1.0e38f
#endif

//global instance
Drawings::Drawing *drawing = NULL;

namespace Drawings
{

//================ drawing parameters ================
#define SPH_FACTOR  0.7f
#define FILL_FACTOR 0.35f

float WIDTH_LINE;
float WIDTH_BORDER;
#define BASIC_WIDTH_LINE 4.0f
#define BASIC_WIDTH_BORDER 2.0f

#define LINE_OPACITY   0.3f
#define SOLID_OPACITY  0.7f
#define BASE_DENSITY   0.1f

#define CONTRAST_FACTOR 0.3f
#define TINY_FACTOR 0.25f

//detail params
#define CIRC_SCALE 30.0f
#define SPH_SCALE  240.0f
#define LOUSY_FACTOR 0.8f
#define LINE_SCALE 0.004f
#define FACE_SCALE 0.0025f

//#define STRIPED
#define STRIPES 1.0
#define BOWED

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

#define PROJ_W0 1.0000001f

//testing
//#define TEST_DEPTH
#define NUM_BINS 40
float depth_bins[NUM_BINS];

//================ stl file object (stereolithography) ================

/** STL file format for stereolithography
 *
 * (1) facet normals are redundant, and must satisfy right-hand rule.
 * (2) objects must lie entirely in first octant.
 * (3) triangular mesh non self-intersecting.
 * (4) no vertex of one triangle lies within the edge of another triangle.
 *
 * references:
 *   http://mech.fsv.cvut.cz/~dr/papers/Lisbon04/node2.html
 *   http://rpdrc.ic.polyu.edu.hk/old_files/stl_introduction.htm
 */

class STL
{
    std::fstream file;
    //buffer for quad strips
    float buff1[3], buff2[3];
    bool buffered;
public:
    STL (std::string filename);
    ~STL ();

    //internal triangle interface
private:
    void point (float x, float y, float z);
    void triangle (float x1, float y1, float z1,
                   float x2, float y2, float z2,
                   float x3, float y3, float z3);

    //quad strip interface
public:
    void new_quad_strip () { buffered = false; }
    void new_segment (float x1, float y1, float z1,
                      float x2, float y2, float z2);
    inline void new_segment (float* v1, float* v2);
};
STL::STL (std::string filename) : file(filename.c_str(), std::ios_base::out)
{
    Assert(file.is_open(), "failed to open " << filename << " for writing");
    file << "solid jenn3d";
}
STL::~STL ()
{
    if (not file) return;
    file << "\n\nendsolid jenn3d\n";
    file.close();
}
void STL::point (float x, float y, float z)
{
    file << "\n    vertex " << x << ' ' << y << ' ' << z;
}
void STL::triangle (float x1, float y1, float z1,
                    float x2, float y2, float z2,
                    float x3, float y3, float z3)
{
    //compute facet normal
    Vect t12, t13, n;
    t12[0] = x2-x1; t12[1] = y2-y1; t12[2] = z2-z1;
    t13[0] = x3-x1; t13[1] = y3-y1; t13[2] = z3-z1;
    cross3(t12, t13, n);
    if (not normalize3(n)) return; //ignore degenerate faces

    //write facet
    file << "\n\nfacet normal " << n[0] << ' ' << n[1] << ' ' << n[2];
    file << "\n  outer loop";
        point(x1,y1,z1);
        point(x2,y2,z2);
        point(x3,y3,z3);
    file << "\n  endloop";
    file << "\nendfacet";

}
void STL::new_segment (float x1, float y1, float z1,
                       float x2, float y2, float z2)
{
    if (buffered) {
        //draw two triangles forming a quad
        triangle (buff1[0], buff1[1], buff1[2],
                  buff2[0], buff2[1], buff2[2],
                        x1,       y1,       z1);
        triangle (buff2[0], buff2[1], buff2[2],
                        x2,       y2,       z2,
                        x1,       y1,       z1);
    }

    //copy new points two buffer
    buff1[0] = x1; buff1[1] = y1; buff1[2] = z1;
    buff2[0] = x2; buff2[1] = y2; buff2[2] = z2;
    buffered = true;
}
inline void STL::new_segment (float* v1, float* v2)
{
    new_segment (v1[0], v1[1], v1[2],
                 v2[0], v2[1], v2[2]);
}

//================ colors ================
#define START_STATE 1
#define COLOR_LINE    0.5f, 0.5f, 0.5f
#define COLOR_BLACK   0.0f, 0.0f, 0.0f
#define COLOR_WHITE   1.0f, 1.0f, 1.0f
#define COLOR_FACE    0.4f, 0.7f, 1.0f
//#define COLOR_FACE    8.0f, 0.5f, 0.2f
//#define COLOR_FACE    0.2f, 0.5f, 0.8f
float *color_fg;  //current foreground drawing color
float *color_fl;  //current fill color
const float
    C_bg[4] = {COLOR_BG,    LINE_OPACITY},
    C_iv[4] = {COLOR_BG,    0.0f},
    C_ln[4] = {COLOR_LINE,  LINE_OPACITY},
C_bl[4] = {COLOR_BLACK, SOLID_OPACITY},
    C_wh[4] = {COLOR_WHITE, SOLID_OPACITY},
    C_bl_ln[4] = {COLOR_BLACK, 1.0f},
    C_wh_ln[4] = {COLOR_WHITE, 1.0f},
    C_bb[4] = {C_ln[0], //bubble color
               C_ln[1],
               C_ln[2],
               0.5f*LINE_OPACITY};
typedef float* Color;
Color
    c_bg    = const_cast<float*>(C_bg),
    c_iv    = const_cast<float*>(C_iv),
    c_ln    = const_cast<float*>(C_ln),
    c_bl    = const_cast<float*>(C_bl),
    c_wh    = const_cast<float*>(C_wh),
    c_bl_ln = const_cast<float*>(C_bl_ln),
    c_wh_ln = const_cast<float*>(C_wh_ln),
    c_bb    = const_cast<float*>(C_bb);
float color_fc[4] = {COLOR_FACE,  0.0f};
static float _result_color[3];
inline Color blend_colors (Color c1, Color c2, float s1, float s2)
{
    for (int i=0; i<3; ++i) {
        _result_color[i] = s1 * c1[i] + s2 * c2[i];
    }
    return _result_color;
}
inline Color get_color (float t)
{
    float s = (1.0f+CONTRAST_FACTOR) * fabs(t) - CONTRAST_FACTOR;
    return blend_colors(color_fg, color_fl, 1.0f-s, s);
}
inline void blend_colors (Color c1, Color c2,
                          float s1, float s2, float* result)
{
    for (int i=0; i<3; ++i) {
        result[i] = s1 * c1[i] + s2 * c2[i];
    }
}
inline void set_color (float t, float* result)
{
    //float s = 1.3f * fabs(t) - 0.3f;
    float s = fabs(t);
    blend_colors(color_fg, color_fl, 1.0f-s, s, result);
}
inline float modulate (complex t)
{
    return 0.5 * (1.0 + sinf(STRIPES * std::arg(t) + elapsed_time()));
}

//optics & opacity
float base_density = BASE_DENSITY;
inline float optical_density (float dx2)
{//calculates opacity from depth-component of normal vector
    return 1.0f - expf(-base_density * sqrtf(dx2));
}
inline float optical_density_approx (float dx2)
{//approximate optical density calculation
    float t = dx2 * sqr(base_density);
    return t / (1.0f + t);
}

//================ drawing class ================
Drawing::Drawing (ToddCoxeter::Graph* g)
    : go(g),
      graph(*go.graph),
      ord(graph.ord),
      deg(graph.deg),
      ord_f(graph.ord_f),
      w_bound0(-1.0f),
      w_bound1( 1.0f),
      h_bound0(-1.0f),
      h_bound1( 1.0f),
      cmp(ord),
      cmp_f(ord_f),
      sorted(ord, 0),
      sorted_f(ord_f, 0),
      vertices(ord),
      centers(ord),
      radii0(ord, 1.0f),
      scales(ord, 1.0f),
      radii(ord, 1.0f),
      phases(ord, 0.0f),
      faces(graph.faces),
      vertices_f(ord_f),
      normals(ord_f),
      centers_f(ord_f),
      _grid_on(false),
      _drawing_verts(true),
      _drawing_edges(true),
      _drawing_faces(true),
      _fancy(true),
      _hazy(false),
      _wireframe(false),
      _high_quality(false),
      _clipping(true),
      _update_needed(true)
{
    logger.info() << "drawing " << ord << " verts, "
                                << (ord * deg) / 2 << " edges" |0;

    //fill in go board's history
    std::vector<std::pair<float,int> > points_i(graph.ord);
    for (int i=0; i<graph.ord; ++i) {
        const Vect& p = graph.points[i];
        float level = p[0]; //approximately lexicographical in [w,z,y,x]
        for (int j=1; j<4; ++j) {
            level = p[j] + 0.01 * level;
        }
        points_i[i] = std::make_pair(-level,i);
    }
    std::sort(points_i.begin(), points_i.end());
    for (int i=0; i<graph.ord; ++i) {
        go.play(points_i[i].second, START_STATE);
    }
    points_i.resize(0);

    //define standard node radius, all pairs are assumed equidistant
    rad0 = 0.5f * r4_dist(graph.points[0], graph.points[graph.adj[0][0]]);
    float tot_tube_len = graph.ord * graph.deg * rad0;
    set_tube_rad(FILL_FACTOR / sqrtf(tot_tube_len));
    coating = 0.05;

    //define oscillation phases
    for (int v = 0; v < ord; ++v) {
        phases[v] = hopf_phase(graph.points[v]);
    }
    logger.debug() << "oscillation phases defined." |0;

    //define pre-projection matrix
    mat_identity(project);
    logger.debug() << "projection built and set to identity." |0;

    //define depth-sorted list
    for (int v=0; v<ord;   ++v) { sorted  [v] = v; }
    for (int f=0; f<ord_f; ++f) { sorted_f[f] = f; }
    logger.debug() << "sorted built and set to linear order." |0;

    //define original polygon
    for (int i = 0; i<POLY_SIDES; ++i) {
        poly0[i][0] = -sin((2.0f*M_PI*i)/POLY_SIDES);
        poly0[i][1] =  cos((2.0f*M_PI*i)/POLY_SIDES);
    }

    //define original sphere
    for (int i = 0; i<=SPH_RHO; ++i) {
        float rho = (0.5f*M_PI*i) / SPH_RHO;
        float cos_rho = cos(rho);
        float sin_rho = sin(rho);
        for (int j=0; j<SPH_THETA; ++j) {
            float theta = (2.0f*M_PI*j) / SPH_THETA;
            float cos_theta = cos(theta);
            float sin_theta = sin(theta);
            sphere[i][j][0] = sin_rho * cos_theta;
            sphere[i][j][1] = sin_rho * sin_theta;
            sphere[i][j][2] = cos_rho;
        }
    }
}

//interface
void Drawing::set_scale (float _scale)
{
    scale = _scale;
    set_quality (_high_quality);
}
void Drawing::set_tube_rad (float rad)
{
    tube_rad = min(0.8f*rad0, rad);
    tube_rad = max(0.01f*rad0, tube_rad);
    sph_rad = SPH_FACTOR * tube_rad * sqrtf(graph.deg);
    sph_rad = max(tube_rad, sph_rad);
    sph_rad0 = sph_rad / rad0;
    tube_factor = tube_rad / sph_rad0;
}
void Drawing::set_bounds (float w0, float w1, float h0, float h1)
{
    w_bound0 = w0;
    w_bound1 = w1;
    h_bound0 = h0;
    h_bound1 = h1;
}
void Drawing::reproject (Mat& theta)
{
    mat_copy(theta, project);
    for (int v=0; v<ord; ++v) {
        vect_mult(project, graph.points[v], vertices[v]);
        update_vertex(v);
    }
    if (ord_f and _drawing_faces) {
        for (int f=0; f<ord_f; ++f) {
            vect_mult(project, graph.normals[f], normals[f]);
            update_face(f);
        }
    }
    sort();
}
GLenum FILL = GL_FILL;
GLenum LINE_STRIP = GL_LINE_STRIP;
void Drawing::display ()
{
    if (_update_needed) _update();

    //reset params jacked by windows
    glDepthRange(-1.0f, 1.0f);

    if (_wireframe) {
        glLineWidth(1.0f);
        FILL = GL_LINE;
        LINE_STRIP = GL_LINES;
    } else {
        FILL = GL_FILL;
        LINE_STRIP = GL_LINE_STRIP;
    }

    //draw vertices
#ifdef TEST_DEPTH
    if (_fancy) {
        for (int i=0; i<NUM_BINS; ++i) { depth_bins[i] = 0; }
    }
#endif
    if (_fancy or _drawing_faces) glEnable (GL_DEPTH_TEST);
    else                          glDisable(GL_DEPTH_TEST);
    for (int v = 0; v < ord; ++v) {
        int u = sorted[v];
        if (not _grid_on and go.state(u)==0) continue;
        display_vertex(u);
    }
#ifdef TEST_DEPTH
    if (_fancy) {
        for (int i=0; i<NUM_BINS; ++i) { std::cout << depth_bins[i] << "\n"; }
        std::cout << "---------------------------" << std::endl;
    }
#endif

    //draw faces
    if (ord_f and _drawing_faces) {
        glLineWidth(1.0f);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        for (int f = 0; f < ord_f; ++f) {
            int g = sorted_f[f];
            _draw_face(g);
        }
        glDepthMask(GL_TRUE);
    }
}
STL *export_file = NULL; //a single global export file
void Drawing::export_stl ()
{
    if (_update_needed) _update();

    //open file
    export_file = new STL("jenn_export.stl");

    //export
    for (int v = 0; v < ord; ++v) {
        int u = sorted[v];
        if (not _grid_on and go.state(u)==0) continue;
        export_vertex(u);
    }

    delete export_file;
}
int Drawing::select (float x,float y)
{
    if (not _grid_on) return -1;
    for (int n = ord-1; n >= 0; --n) {
        int v = sorted[n];
        if (sqr(x - centers[v][0]) + sqr(y - centers[v][1]) < sqr(radii[v])) {
            return v;
        }
    }
    return -1;
}

//depth function
#ifdef TEST_DEPTH
inline float clamp_depth (float z)
{
    float result = (2.0f/M_PI) * atanf(z);
    ++depth_bins[static_cast<int>((NUM_BINS-1)*0.5f*(result+1))];
    return result;
}
#else
inline float clamp_depth (float z) { return (2.0f/M_PI) * atanf(0.5f*z); }
inline float clamp_depth_alt (float z) { return z / sqrtf(1.0f+z*z); }
#endif

//projections functions
inline float proj (float w)
{
    return fabs(1.0f / (PROJ_W0 + w));
}
inline void stereo_project (Vect& x)
{
    float scale = proj(x[3]);
    x[0] *= scale;
    x[1] *= scale;
    x[2] *= scale;
}
inline float stereo_project (const Vect& x, Vect& pi_x)
{
    float scale = proj(x[3]);
    pi_x[0] = scale * x[0];
    pi_x[1] = scale * x[1];
    pi_x[2] = scale * x[2];
    return scale;
}
inline float stereo_project (const Vect& x, float* pi_x)
{
    float scale = proj(x[3]);
    pi_x[0] = scale * x[0];
    pi_x[1] = scale * x[1];
    pi_x[2] = clamp_depth(scale * x[2]);
    return scale;
}
inline void stereo_project (const Vect& x, const Vect& dx, Vect& y)
{//projects position and surface cross-section
    float s = proj(x[3]);
    y[0] = s * x[0];
    y[1] = s * x[1];
    y[2] = s * x[2];

    float dw = - dx[3];
    float da2 = sqr(dx[0] + dw * y[0]);
    float db2 = sqr(dx[1] + dw * y[1]);
    float dc2 = sqr(dx[2] + dw * y[2]);

    y[3] = (da2 + db2 + dc2) / dc2; //depth component
}
inline float stereo_project (const Vect& x, const Vect& dx,
                             Vect& y, Vect& N, Vect& B)
{//projects a tangent vector : R^3 >--> S^3 --> R^3 to normal vectors
    //map to tangent space of S^3
    float s = inner(x,dx);
    Vect pi;
    for (int i=0; i<4; ++i) pi[i] = dx[i] - s*x[i];
    //don't bother to normalize

    //stereo project back to R^3
    // pi(x)_i = x_i / (1+x_3)
    // pi(x + dx)_i = dx_i / (1+x_3) - x_i dx_3 / (1+x_3)^2
    //              = dx_i / (1+x_3) - pi(x)_i dx_3 / (1+x_3)
    float scale = proj(x[3]);
    y[0] = scale * x[0];
    y[1] = scale * x[1];
    y[2] = scale * x[2];
    float p = -pi[3];
    pi[0] += p * y[0];
    pi[1] += p * y[1];
    pi[2] += p * y[2];

    //find basis for normal space
    cross3(pi, y, N);  N[3] = 0;
    cross3(N, pi, B);  B[3] = 0;
    float n = scale / r3_norm(N);
    float b = scale / r3_norm(B);
    for (int i=0; i<3; ++i) {
        N[i] *= n;
        B[i] *= b;
    }
    return scale; // = norm of N, B
}
float Drawing::get_radius ()
{
    float max_rad = 0;
    Vect projected;
    for (int v=0; v<ord; ++v) {
        stereo_project(graph.points[v], projected);
        max_rad = max(r3_norm(projected), max_rad);
    }
    return max_rad;
}

//================ drawing primitives ================

int Drawing::get_params ()
{
    int params = 0;
    params = (params << 1) + _grid_on;
    params = (params << 1) + _drawing_verts;
    params = (params << 1) + _drawing_edges;
    params = (params << 1) + _drawing_faces;
    params = (params << 1) + _fancy;
    params = (params << 1) + _hazy;
    params = (params << 1) + _wireframe;
    return params;
}
void Drawing::set_params (int params)
{
    //reverse order from above!
    _wireframe      = params & 1;   params >>= 1;
    _hazy           = params & 1;   params >>= 1;
    _fancy          = params & 1;   params >>= 1;
    _drawing_faces  = params & 1;   params >>= 1;
    _drawing_edges  = params & 1;   params >>= 1;
    _drawing_verts  = params & 1;   params >>= 1;
    _grid_on        = params & 1;   params >>= 1;
    update();
}
void Drawing::toggle_fancy () { _fancy = not _fancy; update(); }
void Drawing::toggle_hazy  () { _hazy  = not _hazy;  update(); }
void Drawing::toggle_wireframe () { _wireframe = not _wireframe; update(); }
void Drawing::set_quality (bool quality)
{
    _high_quality = quality;
    q_scale = (_high_quality ? 4.0f : 1.0f) * scale;
    update();
}
void Drawing::_update ()
{
    base_density = _fancy ? BASE_DENSITY : 2.0f * BASE_DENSITY;

    glShadeModel(GL_SMOOTH);
    glEnable(GL_MULTISAMPLE);
    if (_fancy) {
        c_bl = const_cast<float*>(C_bl_ln);
        c_wh = const_cast<float*>(C_wh_ln);
        //glDisable(GL_BLEND);
        glEnable(GL_MULTISAMPLE);
    } else {
        c_bl = const_cast<float*>(C_bl);
        c_wh = const_cast<float*>(C_wh);
        //glEnable(GL_BLEND);
    }

    if (_hazy) {
        glEnable(GL_FOG);
        glFogfv(GL_FOG_COLOR, c_bg);
        glFogf(GL_FOG_START, 0.0f);
        glFogf(GL_FOG_END, 2.0f);
        glFogi(GL_FOG_MODE, GL_LINEAR);
        //glFogfv(GL_FOG_DENSITY, GL_EXP);
        //glFogf(GL_FOG_DENSITY, 0.5f);
    } else {
        glDisable(GL_FOG);
    }

    _update_needed = false;
}

//non-isotropic shading function for tubes
inline float max_z (Vect &u, Vect &v, float scale)
{
    return scale * sqrtf(1.0f-sqr((u[0]*v[1]-u[1]*v[0]) / sqr(scale)));
}

//face/edge subdivision tools
int Drawing::_num_segments (float w, float dist)
{
    float detail = LINE_SCALE * sqrtf(q_scale) * dist * proj(w);
    int segs = int(1.0f + LINE_SIDES * detail);
    int line_sides = _high_quality ? LINE_SIDES : LINE_SIDES / 2;
    if (segs > line_sides) segs = line_sides;
    return segs;
}
int Drawing::_secant_stride (float w, float rad)
{
    float radius = rad * proj(w);
    int step = int(2.0f + CIRC_SCALE/sqrtf(radius * q_scale));
    if (step > 10) step = 10;
    return step;
}
int Drawing::_num_subdivs (float w, int Nfaces)
{
    float detail = Nfaces *FACE_SCALE *sqrtf(q_scale) *rad0 *powf(proj(w),0.8);
    int sides = int(1.0f + FACE_SIDES * detail);
    int face_sides = _high_quality ? FACE_SIDES : FACE_SIDES / 2;
    sides = min(sides, face_sides);
    return sides;
}

//================ drawing features ================
void Drawing::_draw_bulb (float* center, float radius)
{
    //calculate detail
    int step = int(1.0f + CIRC_SCALE/sqrtf(fabs(radius) * q_scale));
    if (step > 10) step = 10;

    //define transformed polygon
    float outer = radius * 1.0f;
    float inner = radius * 0.9f;
    float depth = clamp_depth(center[2]);
    for (unsigned i = 0; i < POLY_SIDES; i+=step) {
        poly1[i][0] = center[0] + inner * poly0[i][0];
        poly1[i][1] = center[1] + inner * poly0[i][1];
        poly1[i][2] = depth;
        poly2[i][0] = center[0] + outer * poly0[i][0];
        poly2[i][1] = center[1] + outer * poly0[i][1];
        poly2[i][2] = depth;
    }
    glLineWidth(1.5f);
    glPolygonMode(GL_FRONT_AND_BACK, FILL);

    //draw filled center
    glColor4fv(color_fl);
    glBegin(GL_POLYGON);
    for (unsigned i = 0; i < POLY_SIDES; i+=step) {
        glVertex3fv(poly1[i]);
    }
    glEnd();

    //draw outline
    glColor4fv(color_fg);
    glBegin(GL_QUAD_STRIP);
    for (unsigned i = 0; i < POLY_SIDES; i+=step) {
        glVertex3fv(poly1[i]);
        glVertex3fv(poly2[i]);
    }
    glVertex3fv(poly1[0]);
    glVertex3fv(poly2[0]);
    glEnd();

    /*
    //draw antialiased outlines
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_POLYGON);
    for (int i = 0; i < POLY_SIDES; i+=step) {
        glVertex3fv(poly2[i]);
    }
    glEnd();
    glColor4fv(color_fl);
    glBegin(GL_POLYGON);
    for (int i = 0; i < POLY_SIDES; i+=step) {
        glVertex3fv(poly1[i]);
    }
    glEnd();
    */
}
void Drawing::_draw_sphere (float* center, float radius, int v)
{
    //calculate detail
    int step = int(1 + SPH_SCALE/(fabs(radius) * q_scale));
    if (step > 5) step = 6;
    else if (step > 4) step = 4;

    //drawing flags
    glPolygonMode(GL_FRONT, FILL);
    glShadeModel(GL_SMOOTH);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    if (radius < 0) glFrontFace(GL_CW);

    for (unsigned i=step; i<=SPH_RHO; i += step) {
        float inner_color[3], outer_color[3];
#ifdef STRIPED
        float mod = modulate(phases[v]);
        set_color(mod * sphere[i-step][0][2], inner_color);
        set_color(mod * sphere[  i   ][0][2], outer_color);
#else
        set_color(sphere[i-step][0][2], inner_color);
        set_color(sphere[  i   ][0][2], outer_color);
#endif
        float inner_depth = clamp_depth(center[2]+radius*sphere[i-step][0][2]);
        float outer_depth = clamp_depth(center[2]+radius*sphere[  i   ][0][2]);

        glBegin(GL_QUAD_STRIP);
        for (unsigned j=0; j<SPH_THETA; j+=step) {
            glColor3fv(inner_color);
            glVertex3f(center[0] + radius * sphere[i-step][j][0],
                       center[1] + radius * sphere[i-step][j][1],
                       inner_depth);
            glColor3fv(outer_color);
            glVertex3f(center[0] + radius * sphere[i][j][0],
                       center[1] + radius * sphere[i][j][1],
                       outer_depth);
        }
        glColor3fv(inner_color);
        glVertex3f(center[0] + radius * sphere[i-step][0][0],
                   center[1] + radius * sphere[i-step][0][1],
                   inner_depth);
        glColor3fv(outer_color);
        glVertex3f(center[0] + radius * sphere[i][0][0],
                   center[1] + radius * sphere[i][0][1],
                   outer_depth);
        glEnd();
    }

    glDisable(GL_CULL_FACE);
    if (radius < 0) glFrontFace(GL_CCW);
}
void Drawing::_export_sphere (float* center, float radius, int v)
{
    radius += coating;

    //calculate detail
    int step = int(1 + SPH_SCALE/(fabs(radius) * q_scale));
    if (step > 5) step = 6;
    else if (step > 4) step = 4;

    //draw front and back faces
    float sign = 1.0f;
    for (int side = 0; side <=1; ++side) {
        sign = -sign;

        for (unsigned i=step; i<=SPH_RHO; i += step) {
            float inner_depth = center[2] + sign*radius*sphere[i-step][0][2];
            float outer_depth = center[2] + sign*radius*sphere[  i   ][0][2];

            export_file->new_quad_strip();
            for (int j=0; j<SPH_THETA; j+=step) {
                export_file->new_segment(
                    center[0] + sign * radius * sphere[i-step][j][0],
                    center[1] + radius * sphere[i-step][j][1],
                    inner_depth,
                    center[0] + sign * radius * sphere[i][j][0],
                    center[1] + radius * sphere[i][j][1],
                    outer_depth
                );
            }
            export_file->new_segment(
                    center[0] + sign * radius * sphere[i-step][0][0],
                    center[1] + radius * sphere[i-step][0][1],
                    inner_depth,
                    center[0] + sign * radius * sphere[i][0][0],
                    center[1] + radius * sphere[i][0][1],
                    outer_depth
            );
        }
    }
}
void Drawing::_draw_arc (Vect& begin, Vect& end, float w)
{//draws a line-based arc from far to near

    //check whether the tube needs to be drawn
    if (_clipping) { //clipping fails when panning
        float s0 = 1/(1+begin[3]);
        float s1 = 1/(1+end[3]);
        float bx = s0 * begin[0];
        float by = s0 * begin[1];
        float ex = s1 * end[0];
        float ey = s1 * end[1];
        if ((bx > w_bound1) and (ex > w_bound1)) return;
        if ((bx < w_bound0) and (ex < w_bound0)) return;
        if ((by > h_bound1) and (ey > h_bound1)) return;
        if ((by < h_bound0) and (ey < h_bound0)) return;
    }

    //calculate scale
    int S = _num_segments(w, rad0);
    float line_scale = LINE_SCALE * scale * rad0 * proj(w);
    if (line_scale > 1) line_scale = 1;
    WIDTH_LINE = line_scale * BASIC_WIDTH_LINE;
    glLineWidth(WIDTH_LINE);
    glColor4fv(color_fl);

    float point[3];
    if (S == 1) {
        //draw line
        glBegin(GL_LINES);
            stereo_project(begin, point);  glVertex3fv(point);
            stereo_project(end,   point);  glVertex3fv(point);
        glEnd();
    } else {
        //calculate segment locations
        glBegin(LINE_STRIP);
        for (int s=0; s<=S; ++s) {
            Vect temp;
            for (int i=0; i<4; ++i) {
                temp[i] = s * begin[i] + (S-s) * end[i];
            }
            normalize(temp);
            stereo_project(temp, point);
            glVertex3fv(point);
        }
        glEnd();
    }
}
void Drawing::_draw_arc2 (Vect& begin, Vect& end, float w)
{//draws a line-based arc from far to near
    //check whether the tube needs to be drawn
    if (_clipping) { //clipping fails when panning
        float s0 = 1/(1+begin[3]);
        float s1 = 1/(1+end[3]);
        float bx = s0 * begin[0];
        float by = s0 * begin[1];
        float ex = s1 * end[0];
        float ey = s1 * end[1];
        if ((bx > w_bound1) and (ex > w_bound1)) return;
        if ((bx < w_bound0) and (ex < w_bound0)) return;
        if ((by > h_bound1) and (ey > h_bound1)) return;
        if ((by < h_bound0) and (ey < h_bound0)) return;
    }

    //calculate scale
    int S = _num_segments(w, rad0);
    float line_scale = LINE_SCALE * q_scale * rad0 * proj(w);
    if (line_scale > 1) line_scale = 1;
    WIDTH_LINE   = line_scale * BASIC_WIDTH_LINE;
    WIDTH_BORDER = line_scale * BASIC_WIDTH_BORDER;

    if (S == 1) {
        //calculate line location
        stereo_project(begin, lines[0]);
        stereo_project(end,   lines[1]);

        //draw foreground border
        glLineWidth(WIDTH_LINE + 2*WIDTH_BORDER);
        glColor4fv(color_fg);
        glBegin(GL_LINES);
            glVertex3fv(lines[0]);
            glVertex3fv(lines[1]);
        glEnd();

        //center fill
        //glEnable(GL_POLYGON_OFFSET_LINE);
        glLineWidth(WIDTH_LINE);
        glColor4fv(color_fl);
        glBegin(GL_LINES);
            glVertex3fv(lines[0]);
            glVertex3fv(lines[1]);
        glEnd();
        //glDisable(GL_POLYGON_OFFSET_LINE);
    } else {
        //calculate segment locations
        for (int s=0; s<=S; ++s) {
            Vect temp;
            for (int i=0; i<4; ++i) {
                temp[i] = s * begin[i] + (S-s) * end[i];
            }
            normalize(temp);
            stereo_project(temp, lines[s]);
        }

        //draw foreground border
        glLineWidth(WIDTH_LINE + 2*WIDTH_BORDER);
        glColor4fv(color_fg);
        glBegin(LINE_STRIP);
        for (int s=0; s<=S; ++s) {
            glVertex3fv(lines[s]);
        }
        glEnd();

        //center fill
        //glEnable(GL_POLYGON_OFFSET_LINE);
        glLineWidth(WIDTH_LINE);
        glColor4fv(color_fl);
        glBegin(LINE_STRIP);
        for (int s=0; s<=S; ++s) {
            glVertex3fv(lines[s]);
        }
        glEnd();
        //glDisable(GL_POLYGON_OFFSET_LINE);
    }
}
void Drawing::_draw_arc_wide (Vect& begin, Vect& end, float w)
{
    //calculate scale
    int S = _num_segments(w, rad0);

    //calculate segment locations
    for (int s=0; s<=S; ++s) {
        Vect temp;
        for (int i=0; i<4; ++i) {
            temp[i] = s * begin[i] + (S-s) * end[i];
        }
        normalize(temp);
        width[s] = tube_rad * stereo_project(temp, lines[s]);
    }

    //calculate normals
    for (int s=0; s<S; ++s) {
        float dx = lines[s+1][1] - lines [s] [1];
        float dy = lines [s] [0] - lines[s+1][0];
        float r = sqrtf(sqr(dx)+sqr(dy));
        if (r > 0) {
            dx /= r;
            dy /= r;
        }
        bord1[s][0] = dx;
        bord1[s][1] = dy;
    }
    if (S == 1) {
        normal[0][0] = normal[1][0] = bord1[1][0];
        normal[0][1] = normal[1][1] = bord1[1][1];
    } else {
        //extrapolate endpoints
        normal[0][0] = 1.5f*bord1[0][0] - 0.5f*bord1[1][0];
        normal[0][1] = 1.5f*bord1[0][1] - 0.5f*bord1[1][1];
        normal[S][0] = 1.5f*bord1[S-1][0] - 0.5f*bord1[S-2][0];
        normal[S][1] = 1.5f*bord1[S-1][1] - 0.5f*bord1[S-2][1];
        //interpolate interior
        for (int s=1; s<S; ++s) {
            normal[s][0] = 0.5f * (bord1[s-1][0] + bord1[s][0]);
            normal[s][1] = 0.5f * (bord1[s-1][1] + bord1[s][1]);
        }
    }

    //find borders
    for (int s=0; s<=S; ++s) {
        float x = lines[s][0];
        float y = lines[s][1];
        float dx = width[s] * normal[s][0];
        float dy = width[s] * normal[s][1];
        bord1[s][0] = x - dx;
        bord1[s][1] = y - dy;
        bord2[s][0] = x + dx;
        bord2[s][1] = y + dy;
    }

    //draw foreground border
    glPolygonMode(GL_FRONT_AND_BACK, FILL);
    /*
    glColor4fv(color_fg);
    glBegin(GL_QUAD_STRIP);
    for (int s=0; s<=S; ++s) {
        glVertex2fv(bord1[s]);
        glVertex2fv(bord2[s]);
    }
    glEnd();
    */

    //center fill
    glLineWidth(WIDTH_LINE);
    glColor4fv(color_fl);
    glBegin(GL_QUAD_STRIP);
    for (int s=0; s<=S; ++s) {
        glVertex2fv(bord1[s]);
        glVertex2fv(bord2[s]);
    }
    glEnd();
}
void Drawing::_draw_tube (Vect& begin, Vect& end,
                          float r0, float r1, float w, int v0, int v1)
{
    //check whether the tube needs to be drawn
    if (_clipping) { //clipping fails when panning
        float s0 = 1/(1+begin[3]);
        float s1 = 1/(1+end[3]);
        float bx = s0 * begin[0];
        float by = s0 * begin[1];
        float ex = s1 * end[0];
        float ey = s1 * end[1];
        float R0 = s0 * r0;
        float R1 = s1 * r1;
        if ((bx - R0 > w_bound1) and (ex - R1 > w_bound1)) return;
        if ((bx + R0 < w_bound0) and (ex + R1 < w_bound0)) return;
        if ((by - R0 > h_bound1) and (ey - R1 > h_bound1)) return;
        if ((by + R0 < h_bound0) and (ey + R1 < h_bound0)) return;
    }

    //calculate scale
    int S = _num_segments(w, 2*rad0);
    int step = _secant_stride (w, max(r0,r1));

    //define tangents
    Vect tangent;
    for (int i=0; i<4; ++i) tangent[i] = end[i] - begin[i];
    normalize(tangent);

    //drawing flags
    glPolygonMode(GL_FRONT, FILL);
    glShadeModel(GL_SMOOTH);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    //loop through cylinders
    for (int s=0; s<S; ++s) {
        if (s) {
            //copy previous cylinder
            //slower version
            //  for (int i = 0; i < POLY_SIDES; i+=step) {
            //      poly1[i][0] = poly2[i][0];
            //      poly1[i][1] = poly2[i][1];
            //      poly1[i][2] = poly2[i][2];
            //      shade1[i]   = shade2[i];
            //  }
            //faster version
            memcpy(poly1, poly2, 3*POLY_SIDES*sizeof(float));
            memcpy(shade1, shade2, POLY_SIDES*sizeof(float));
        } else {
            //define back face
            Vect point1;
            for (int i=0; i<4; ++i) {
                point1[i] = s * end[i] + (S-s) * begin[i];
            }
            normalize(point1);
            Vect center, du1, dv1;
            float scale1 = stereo_project(point1, tangent, center, du1, dv1);
            float max_z1 = 0.83f * max_z(du1, dv1, scale1) + 0.17f * scale1;
            du1[3] = du1[2] / max_z1;
            dv1[3] = dv1[2] / max_z1;
#ifdef BOWED
            float rad = sqrtf(sqr(s * r1) + sqr((S-s) * r0))/S;
#else
            float rad = (s * r1 + (S-s) * r0)/S;
#endif
            du1[0] *= rad; du1[1] *= rad; du1[2] *= rad;
            dv1[0] *= rad; dv1[1] *= rad; dv1[2] *= rad;
#ifdef STRIPED
            float a =  s ; a/= S;
            float b = S-s; b/= S;
            complex phase = a * phases[v1] + b * phases[v0];
            float mod = modulate(phase);
#endif
            for (unsigned i = 0; i < POLY_SIDES; i+=step) {
                //define points cylinder
                poly1[i][0] = center[0]
                            + poly0[i][0] * du1[0]
                            + poly0[i][1] * dv1[0];
                poly1[i][1] = center[1]
                            + poly0[i][0] * du1[1]
                            + poly0[i][1] * dv1[1];
                poly1[i][2] = clamp_depth( center[2] +
                                         + poly0[i][0] * du1[2]
                                         + poly0[i][1] * dv1[2]);
                shade1[i]   = poly0[i][0] * du1[3]
                            + poly0[i][1] * dv1[3];
#ifdef STRIPED
                shade1[i] *= mod;
#endif
            }
        }

        //define front face
        Vect point2;
        for (int i=0; i<4; ++i) {
            point2[i] = (s+1) * end[i] + (S-s-1) * begin[i];
        }
        normalize(point2);
        Vect center2, du2, dv2;
        float scale2 = stereo_project(point2, tangent, center2, du2, dv2);
        float max_z2 = 0.83f * max_z(du2, dv2, scale2) + 0.17f * scale2;
        du2[3] = du2[2] / max_z2;
        dv2[3] = dv2[2] / max_z2;
#ifdef BOWED
        float rad = sqrtf(sqr((s+1) * r1) + sqr((S-s-1) * r0))/S;
#else
        float rad = ((s+1) * r1 + (S-s-1) * r0)/S;
#endif
        du2[0] *= rad; du2[1] *= rad; du2[2] *= rad;
        dv2[0] *= rad; dv2[1] *= rad; dv2[2] *= rad;
#ifdef STRIPED
        float a =  s ; a/= S;
        float b = S-s; b/= S;
        complex phase = a * phases[v1] + b * phases[v0];
        float mod = modulate(phase);
#endif
        for (unsigned i = 0; i < POLY_SIDES; i+=step) {
            //define points cylinder
            poly2[i][0] = center2[0]
                        + poly0[i][0] * du2[0]
                        + poly0[i][1] * dv2[0];
            poly2[i][1] = center2[1]
                        + poly0[i][0] * du2[1]
                        + poly0[i][1] * dv2[1];
            poly2[i][2] = clamp_depth( center2[2] +
                                     + poly0[i][0] * du2[2]
                                     + poly0[i][1] * dv2[2]);
            shade2[i]   = poly0[i][0] * du2[3]
                        + poly0[i][1] * dv2[3];
#ifdef STRIPED
            shade2[i] *= mod;
#endif
        }

        //draw cylinder
        glBegin(GL_QUAD_STRIP);
        for (unsigned i = 0; i < POLY_SIDES; i+=step) {
            glColor3fv(get_color(shade1[i]));  glVertex3fv(poly1[i]);
            glColor3fv(get_color(shade2[i]));  glVertex3fv(poly2[i]);
        }
        glColor3fv(get_color(shade1[0]));  glVertex3fv(poly1[0]);
        glColor3fv(get_color(shade2[0]));  glVertex3fv(poly2[0]);
        glEnd();
    }

    glDisable(GL_CULL_FACE);
}
void Drawing::_export_tube (Vect& begin, Vect& end,
                            float r0, float r1, float w, int v0, int v1)
{
    //calculate scale
    int S = _num_segments(w, 2*rad0);
    int step = _secant_stride (w, max(r0,r1));

    //define tangents
    Vect tangent;
    for (int i=0; i<4; ++i) tangent[i] = end[i] - begin[i];
    normalize(tangent);

    //loop through cylinders
    for (int s=0; s<S; ++s) {
        if (s) {
            //copy previous cylinder
            memcpy(poly1, poly2, 3*POLY_SIDES*sizeof(float));
        } else {
            //define back face
            Vect point1;
            for (int i=0; i<4; ++i) {
                point1[i] = s * end[i] + (S-s) * begin[i];
            }
            normalize(point1);
            Vect center, du1, dv1;
            float scale1 = stereo_project(point1, tangent, center, du1, dv1);
            float max_z1 = 0.83f * max_z(du1, dv1, scale1) + 0.17f * scale1;
            du1[3] = du1[2] / max_z1;
            dv1[3] = dv1[2] / max_z1;
#ifdef BOWED
            float rad = sqrtf(sqr(s * r1) + sqr((S-s) * r0))/S;
#else
            float rad = (s * r1 + (S-s) * r0)/S;
#endif
            rad += coating / scale1;
            du1[0] *= rad; du1[1] *= rad; du1[2] *= rad;
            dv1[0] *= rad; dv1[1] *= rad; dv1[2] *= rad;
            for (unsigned i = 0; i < POLY_SIDES; i+=step) {
                //define points cylinder
                for (int j = 0; j<3; ++j) {
                    poly1[i][j] = center[j]
                                + poly0[i][0] * du1[j]
                                + poly0[i][1] * dv1[j];
                }
            }
        }

        //define front face
        Vect point2;
        for (int i=0; i<4; ++i) {
            point2[i] = (s+1) * end[i] + (S-s-1) * begin[i];
        }
        normalize(point2);
        Vect center2, du2, dv2;
        float scale2 = stereo_project(point2, tangent, center2, du2, dv2);
        float max_z2 = 0.83f * max_z(du2, dv2, scale2) + 0.17f * scale2;
        du2[3] = du2[2] / max_z2;
        dv2[3] = dv2[2] / max_z2;
#ifdef BOWED
        float rad = sqrtf(sqr((s+1) * r1) + sqr((S-s-1) * r0))/S;
#else
        float rad = ((s+1) * r1 + (S-s-1) * r0)/S;
#endif
        rad += coating / scale2;
        du2[0] *= rad; du2[1] *= rad; du2[2] *= rad;
        dv2[0] *= rad; dv2[1] *= rad; dv2[2] *= rad;
        for (unsigned i = 0; i < POLY_SIDES; i+=step) {
            //define points cylinder
            for (int j = 0; j<3; ++j) {
                poly2[i][j] = center2[j]
                            + poly0[i][0] * du2[j]
                            + poly0[i][1] * dv2[j];
            }
        }

        //draw cylinder
        export_file->new_quad_strip();
        for (unsigned i = 0; i < POLY_SIDES; i+=step) {
            export_file->new_segment(poly1[i], poly2[i]);
        }
        export_file->new_segment(poly1[0], poly2[0]);
    }
}
inline void project_face (const Vect& v, const Vect& n, Vect& c)
{
    stereo_project(v, n, c);
    c[2] = clamp_depth(c[2]);
    c[3] = optical_density(c[3]);
}
inline void draw_face_vert (const Vect& xyz_a)
{//draws a vertex in (x,y,z,alpha) format
    color_fc[3] = xyz_a[3];
    glColor4fv(color_fc);
    glVertex3fv(xyz_a.data);
}
void Drawing::_draw_face (int f)
{
    const Face& face = faces[f];
    int N = face.size();

    //check whether face should be drawn
    for (int n=0; n<N; ++n) {
        if (not go.state(face[n])) return;
    }
    if (_clipping) { //clipping fails when panning
        int hidden = 1+2+4+8;
        for (int n=0; n<N; ++n) {
            const Vect& v = centers[face[n]];
            if (v[0] < w_bound1) hidden &= 0+2+4+8;
            if (v[0] > w_bound0) hidden &= 1+0+4+8;
            if (v[1] < h_bound1) hidden &= 1+2+0+8;
            if (v[1] > h_bound0) hidden &= 1+2+4+0;
            if (not hidden) break;
        }
        if (hidden) return;
    }

    //find center & normal of polygon
    int subdivs = _num_subdivs(centers_f[f][3], N);
    const Vect& vert = vertices_f[f];
    const Vect& normal = normals[f];
    Vect center;
    project_face(vert, normal, center);

    //drawing flags
    glPolygonMode(GL_FRONT_AND_BACK, FILL);
    glShadeModel(GL_SMOOTH);
    glDisable(GL_CULL_FACE);

    if (subdivs == 1) {
        //find corners
        Vect corners[N];
        for (int n=0; n<N; ++n) {
            project_face(vertices[face[n]], normal, corners[n]);
        }

        //draw a fan
        glBegin(GL_TRIANGLE_FAN);
        draw_face_vert(center);
        for(int n=0; n<N; ++n) {
            draw_face_vert(corners[n]);
        }
        draw_face_vert(corners[0]);
        glEnd();
    } else {
        //set up corner array
        int max_sides = N * FACE_SIDES;
        static int num_sides = 0;
        static Vect *corn1 = NULL, *corn2 = NULL; //inner & outer corners
        if (num_sides < max_sides) {
            if (num_sides) { delete[] corn1; delete[] corn2; }
            num_sides = max_sides;
            corn1 = new Vect[num_sides];
            corn2 = new Vect[num_sides];
        }

        //find corners
        for (int n=0; n<N; ++n) {
            Vect corn = corn2[n];
            vect_scale(corn, subdivs-1, vert);
            corn += vertices[face[n]];
            normalize(corn);
            project_face(corn, normal, corn2[n]);
        }

        //draw a fan at center
        glBegin(GL_TRIANGLE_FAN);
        draw_face_vert(center);
        for (int n=0; n<N; ++n) {
            draw_face_vert(corn2[n]);
        }
        draw_face_vert(corn2[0]);
        glEnd();

        //draw radiating anula
        for (int s=1; s<subdivs; ++s) {
            int a0 = subdivs - s - 1;
            int T1 = s, T2 = 1+s;

            //copy inner positions
            for (int n=0; n<N; ++n) {
                //slower version
                //  for (int t=0; t<T1; ++t) {
                //      corn1[n*T2+t] = corn2[n*T1+t];
                //  }
                //faster version
                memcpy(corn1+n*T2, corn2+n*T1, T1*sizeof(Vect));
                corn1[n*T2+T1] = corn2[((n+1)*T1)%(N*T1)];
            }

            //find new corners
            for (int n=0; n<N; ++n) {
                const Vect &left  = vertices[face[n]];
                const Vect &right = vertices[face[(n+1)%N]];
                for (int t=0; t<T2; ++t) {
                    float a2 = t;
                    float a1 = subdivs - a0 - a2;
                    Vect corn;
                    //slower version
                    //  vect_scale(corn, a0, vert);
                    //  vect_isadd(corn, a1, left);
                    //  vect_isadd(corn, a2, right);
                    //  normalize(corn);
                    //faster version
                    float nc=0.0f;
                    for (int i=0; i<4; ++i) {
                        nc += sqr(corn[i] = a0 * vert[i]
                                          + a1 * left[i]
                                          + a2 * right[i]);
                    }
                    nc = 1.0f / sqrtf(nc);
                    for (int i=0; i<4; ++i) corn[i] *= nc;
                    project_face(corn, normal, corn2[n*T2+t]);
                }
            }

            //draw strips
            glBegin(GL_TRIANGLE_STRIP);
            for (int nt=0,NT=N*T2; nt<NT; ++nt) {
                draw_face_vert(corn2[nt]);
                draw_face_vert(corn1[nt]);
            }
            draw_face_vert(corn2[0]);
            glEnd();
        }
    }
}

//vertex drawing
void Drawing::display_vertex (int v)
{
    //set drawing parameters
    float radius = radii[v];
    float radius0 = radii0[v];
    if (_fancy or not _drawing_verts) {
        radius0 = 0;
        //this doesn't account for nonlinearity far from the origin
        //if (radius0 <= tube_rad) radius0 = 0.0f;
        //else                     radius0 = sqrt(sqr(radius0) - sqr(tube_rad));
    } else {
        if (not _drawing_verts) radius0 *= 1e-4f;
    }

    int my_state = go.state(v);

    //update edges
    std::pair<float,int> ordered_lines[deg];
    if (_drawing_edges) {
        float a0 = 2.0f - radius0, a1 = radius0;
        for (int j = 0; j < deg; ++j) {
            int v1 = graph.adj[v][j];
            for (int i = 0; i < 4; ++i) {
                midpoint[j][i] =      vertices[v][i] +      vertices[v1][i];
                contact [j][i] = a0 * vertices[v][i] + a1 * vertices[v1][i];
                farpoint[j][i] = a1 * vertices[v][i] + a0 * vertices[v1][i];
            }
            normalize(midpoint[j]);
            normalize(contact[j]);
            normalize(farpoint[j]);
            w_val[j] = min(midpoint[j][3], contact[j][3]);
            w_val[j] = min(w_val[j], farpoint[j][3]);

            //order
            float z = contact[j][2] * proj(contact[j][3]);
            ordered_lines[j] = std::pair<float,int>(z,j);
        }
        std::sort(ordered_lines, ordered_lines+deg);
    }

    if (_drawing_edges) {
        //draw background lines
        for (int unordered_j = 0; unordered_j < deg; ++unordered_j) {
            //check depth
            float z = ordered_lines[unordered_j].first;
            int   j = ordered_lines[unordered_j].second;
            if (z > centers[v][2]) continue;
            if (_fancy and ( farpoint[j][2] * proj(farpoint[j][3])
                           < contact[j][2] * proj(contact[j][3]) ))
                continue;

            //decide whether edge should be drawn
            int other_state = go.state(graph.adj[v][j]);
            if ((not _grid_on) and (other_state!=my_state)) continue;

            //decide edge coloring & WIDTH_LINE
            int edge_state = (1<<my_state) + (1<<other_state);
            switch (edge_state) {
            case 4:  color_fg = c_wh_ln; color_fl = c_bl_ln; break;
            case 8:  color_fg = c_bl_ln; color_fl = c_wh_ln; break;
            default: color_fg = c_bg;    color_fl = c_ln;    break; //2,3,4,6
            }

            //draw lines
            float w = w_val[j];
            if (_fancy) {
                int v0 = graph.adj[v][j];
                int v1 = v;
                float rad0 = tube_factor * radii0[v0];
                float rad1 = tube_factor * radii0[v1];
                _draw_tube(farpoint[j], contact[j], rad0, rad1, w, v0, v1);
            } else {
                if (_drawing_faces) _draw_arc (midpoint[j], contact[j], w);
                else                _draw_arc2(midpoint[j], contact[j], w);
            }
        }
    }
    if (_drawing_verts) {
        Vect& c = centers[v];
        if ( radius < 0 or (not _clipping) or
             ( (w_bound0 - radius <= c[0]) and (c[0] <= radius + w_bound1)
                and
               (h_bound0 - radius <= c[1]) and (c[1] <= radius + h_bound1) ) ) {
            //draw circle
            //  decide color
            switch (my_state) {
            case 0: color_fg = c_ln; color_fl = c_bb; break;
            case 1: color_fg = c_wh; color_fl = c_bl; break;
            case 2: color_fg = c_bl; color_fl = c_wh; break;
            default:
                logger.error() << "unknown vertex state: " << my_state |0;
            }
            if (_fancy) _draw_sphere(centers[v].data, radius, v);
            else        _draw_bulb  (centers[v].data, radius);
        }
    }

    if (_drawing_edges) {
        //draw foreground lines
        for (int unordered_j = 0; unordered_j < deg; ++unordered_j) {
            //check depth
            float z = ordered_lines[unordered_j].first;
            int   j = ordered_lines[unordered_j].second;
            if (z <= centers[v][2]) continue;
            if (_fancy and ( farpoint[j][2]*proj(farpoint[j][3])
                           < contact[j][2]*proj(contact[j][3]) ))
                continue;

            //decide whether edge should be drawn
            int other_state = go.state(graph.adj[v][j]);
            if ((not _grid_on) and (other_state!=my_state)) continue;

            //decide edge coloring & WIDTH_LINE
            int edge_state = (1<<my_state) + (1<<other_state);
            switch (edge_state) {
            case 4:  color_fg = c_wh_ln; color_fl = c_bl_ln; break;
            case 8:  color_fg = c_bl_ln; color_fl = c_wh_ln; break;
            default: color_fg = c_bg;    color_fl = c_ln;    break; //2,3,4,6
            }

            //draw lines
            float w = w_val[j];
            if (_fancy) {
                int v0 = v;
                int v1 = graph.adj[v][j];
                float rad0 = tube_factor * radii0[v0];
                float rad1 = tube_factor * radii0[v1];
                _draw_tube(farpoint[j], contact[j], rad1, rad0, w, v1, v0);
            } else {
                if (_drawing_faces) _draw_arc (contact[j], midpoint[j], w);
                else                _draw_arc2(contact[j], midpoint[j], w);
            }
        }
    }
}
void Drawing::export_vertex (int v)
{
    //set drawing parameters
    float radius = radii[v];
    int my_state = go.state(v);

    //update edges
    std::pair<float,int> ordered_lines[deg];
    if (_drawing_edges) {
        for (int j = 0; j < deg; ++j) {
            int v1 = graph.adj[v][j];
            for (int i = 0; i < 4; ++i) {
                midpoint[j][i] = vertices[v][i] + vertices[v1][i];
                contact [j][i] = vertices[v][i];
                farpoint[j][i] = vertices[v1][i];
            }
            normalize(midpoint[j]);
            normalize(contact[j]);
            normalize(farpoint[j]);
            w_val[j] = min(midpoint[j][3], contact[j][3]);
            w_val[j] = min(w_val[j], farpoint[j][3]);

            //order
            float z = contact[j][2] * proj(contact[j][3]);
            ordered_lines[j] = std::pair<float,int>(z,j);
        }
        std::sort(ordered_lines, ordered_lines+deg);
    }

    //draw background lines
    if (_drawing_edges) {
        for (int unordered_j = 0; unordered_j < deg; ++unordered_j) {
            //check depth
            float z = ordered_lines[unordered_j].first;
            int   j = ordered_lines[unordered_j].second;
            if (z > centers[v][2]) continue;
            if ( farpoint[j][2] * proj(farpoint[j][3])
               < contact[j][2] * proj(contact[j][3]) ) continue;

            //decide whether edge should be drawn
            int other_state = go.state(graph.adj[v][j]);
            if (other_state!=my_state) continue;

            //draw lines
            float w = w_val[j];
            int v0 = graph.adj[v][j];
            int v1 = v;
            float rad0 = tube_factor * radii0[v0];
            float rad1 = tube_factor * radii0[v1];
            _export_tube(farpoint[j], contact[j], rad0, rad1, w, v0, v1);
        }
    }

    //export vertex
    if (_drawing_verts) {
        _export_sphere(centers[v].data, radius, v);
    }

    //draw foreground lines
    if (_drawing_edges) {
        for (int unordered_j = 0; unordered_j < deg; ++unordered_j) {
            //check depth
            float z = ordered_lines[unordered_j].first;
            int   j = ordered_lines[unordered_j].second;
            if (z <= centers[v][2]) continue;
            if ( farpoint[j][2]*proj(farpoint[j][3])
               < contact[j][2]*proj(contact[j][3]) ) continue;

            //decide whether edge should be drawn
            int other_state = go.state(graph.adj[v][j]);
            if (other_state!=my_state) continue;

            //draw lines
            float w = w_val[j];
            int v0 = v;
            int v1 = graph.adj[v][j];
            float rad0 = tube_factor * radii0[v0];
            float rad1 = tube_factor * radii0[v1];
            _export_tube(farpoint[j], contact[j], rad1, rad0, w, v1, v0);
        }
    }
}
void Drawing::sort (void)
{//using stl's sorting algorithms
    cmp.update(centers);
    std::sort(sorted.begin(), sorted.end(), cmp);

    if (ord_f and _drawing_faces) {
        cmp_f.update(centers_f);
        std::sort(sorted_f.begin(), sorted_f.end(), cmp_f);
    }
}
inline void Drawing::update_vertex (int v)
{
    int s = go.state(v), h = go.highlighted[v];
    float r = sph_rad0;
    r *= _fancy ? (s ? 1.0f : TINY_FACTOR) : LOUSY_FACTOR;
    if (h) {
        float phase = std::arg(phases[v]) + (s==2) * M_PI;
        r *= (1.0f + 0.16f * sin(3*M_PI*elapsed_time() + phase));
    }
    radii0[v] = r;
    r *= rad0;
    if (vertices[v][3] > 0) { //linear projection
        scales[v] = stereo_project(vertices[v], centers[v]);
        radii[v] = r * scales[v];
    } else { //more accurate nonlinear projection
        Vect near = vertices[v], far = near;
        float pos012 = r3_norm(near);
        float pos3 = near[3];
        scales[v] = proj(pos3);
        float diff012 =   r * pos3 / pos012;
        float diff3   = - r * pos012;

        near[0] -= diff012 * near[0];
        near[1] -= diff012 * near[1];
        near[2] -= diff012 * near[2];
        near[3] -= diff3;
        normalize(near);
        stereo_project(near, near);

        far[0] += diff012 * far[0];
        far[1] += diff012 * far[1];
        far[2] += diff012 * far[2];
        far[3] += diff3;
        normalize(far);
        stereo_project(far, far);

        Vect diam;
        for (int i=0; i<3; ++i) {
            centers[v][i] = 0.5f*(near[i] + far[i]);
            diam[i] = far[i] - near[i];
        }
        radii[v] = 0.5f * r3_norm(diam);

        //check for inversion
        if ((vertices[v][3] < -0.9f)
                and (near[0]*far[0] + near[1]*far[1] + near[2]*far[2] < 0)) {
            centers[v][2] = INFINITY;
            radii[v] *= -1.0f;
        }
    }
}
void Drawing::update_face (int f)
{
    //update center for sorting
    const Face& face = faces[f];
    Vect& center = vertices_f[f];
    center = vertices[face[0]];
    for (unsigned n=1; n<face.size(); ++n) {
        center += vertices[face[n]];
    }
    normalize(center);
    stereo_project(center, centers_f[f]);
    float w =  center[3];
    for (unsigned n=1; n<face.size(); ++n) {
        w = min(w, vertices[face[n]][3]);
    }
    centers_f[f][3] = w;
}

}

