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

#include "projection.h"

#ifdef CYGWIN_HACKS
    #define GLUT_STATIC
#endif

#if defined(__APPLE__) && defined(__MACH__)
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

#ifdef CAPTURE
    #include <cstring> //for memcpy
    #include <cstdio> //for fopen, etc
    #include <png.h>
#endif

#define NUM_STILL_FRAMES 128

#define BORDER_RADIUS 1.8f

extern void finish_buffer ();


//global instance
Projection::Projector *projector = NULL;

namespace Projection
{

void Projector::reset ()
{
    paused = false;
    panning = false;
    in_stereo = false;
    high_contrast = false;
    reverse_colors = false;
    aperture_size = 0.01f;
    depth = -0.5f;
    trailing = false;
    trail_paused = false;
    if (trail) { delete trail; trail = NULL; }
#ifdef CAPTURE
    in_color = true;
#endif
    high_contrast = false;
    reverse_colors = false;
    motion_blur = false;
    high_quality = false;
}
void Projector::toggle_quality ()
{
    high_quality = not high_quality;
    drawing->set_quality(high_quality);
    update(false);
}
void Projector::toggle_trail (Trails::Style style) {
    if (trail) { delete trail; trail = NULL; trailing = false; }
    else       { trail = new Trails::Trail(style); trailing = true; }
}
void Projector::toggle_trailing () {
    if (not trail) return;
    trailing = not trailing;
    if (trailing) trail->new_trail();
}
void Projector::toggle_blur ()
{
    motion_blur = not motion_blur;
    update();
}
void Projector::update (bool update_accum)
{
    _update_needed = true;
    _update_accum = update_accum;
    glutPostRedisplay();
}
void Projector::_update ()
{
    W = in_stereo ? w/2 : w;

    float scale = (W+h) / animator->vis_rad;
    drawing->set_scale(scale);

    _bound_image();
    _update_needed = false;
}
void Projector::_bound_image (float x_shift, float y_shift)
{
    float size = sqrt(0.5 * (W*W + h*h));
    w_factor = W / size;
    h_factor = h / size;
    float w_bound = w_factor * animator->vis_rad;
    float h_bound = h_factor * animator->vis_rad;
    float w_bound0 = x_center - w_bound - x_shift;
    float w_bound1 = x_center + w_bound - x_shift;
    float h_bound0 = y_center - h_bound - y_shift;
    float h_bound1 = y_center + h_bound - y_shift;

    drawing->set_bounds(w_bound0, w_bound1, h_bound0, h_bound1);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(w_bound0, w_bound1, h_bound0, h_bound1,
            -1.0f, 1.0f); //near & far clipping planes
    glTranslatef(0.0f,0.0f,1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f,0.0f,-1.0f);
}
void Projector::_draw_buffer ()
{//effects using accumulation buffer
    //blur image
    if (motion_blur and not _update_accum) {
        if (animator->drag_channels) { //very short exposure
            glAccum(GL_MULT,    0.65f);
            glAccum(GL_ACCUM,   0.35f);
        } else { if (high_quality) { //longer exposure
            glAccum(GL_MULT,    0.96f);
            glAccum(GL_ACCUM,   0.04f);
        } else {            //shorter exposure
            glAccum(GL_MULT,    0.85f);
            glAccum(GL_ACCUM,   0.15f);
        } }
    } else {
        glAccum(GL_LOAD,    1.0f);
        _update_accum = false;
    }
}
void Projector::_show_buffer (char* output)
{//effects using accumulation buffer
    //reverse colors
    if (reverse_colors) {
        glAccum(GL_MULT,    -1.0f);
        glAccum(GL_ADD,     1.0f);
    }

    //increase contrast & invert colors
    if (high_contrast) {
        glAccum(GL_ADD,     -0.25f);
        glAccum(GL_RETURN,  2.0f);
        glAccum(GL_ADD,     0.25f);
    } else {
        glAccum(GL_RETURN,  1.0f);
    }

    //draw image as soon as possible
#ifdef CAPTURE
    if (output) _capture_little(output);
#endif
    finish_buffer();

    //restore colors
    if (reverse_colors) {
        glAccum(GL_MULT,    -1.0f);
        glAccum(GL_ADD,     1.0f);
    }
}
void Projector::_set_tilt (float theta, float phi)
{//tilt image by theta,phi
    //rotate model
    mat_rot(2,0,phi,temp1);         //move to edge of aperture
    mat_rot(0,1,theta,temp2);       //find "random" direction for aperture
    mat_conj(temp2,temp1,tilt);

    //correct for depth
    float psi = 2.0f * phi * tanf(depth);
    _bound_image(psi * cosf(theta), -psi * sinf(theta));
}
//we use the first two nobel numbers to minimize comensurability
const float RHO = (sqrtf(5.0f) - 1.0f) / 2.0f;  //the most irrational number
const float RHO2 = sqrtf(3.0f) - 1.0f;          //the 2nd most irrational num.
void Projector::_set_rand_tilt ()
{//find pseudorandom tilt
    if (not motion_blur) { mat_identity(tilt); return; }

    //this generates pseudorandom points in the unit disc
    static int Theta = 0, Phi = 0;
    Theta = (Theta + 1) % 4096;
    Phi   = (Phi   + 1) % 4096;
    float theta = 2.0f * M_PI * fmodf(RHO * Theta, 1.0f);
    float phi = aperture_size * sqrtf(fmodf(RHO2 * Phi, 1.0f));

    _set_tilt(theta, phi);
}
void Projector::_set_unif_tilt (int n, int N)
{//define uniformly random tilt
    float T = (n + 0.5f);
    float theta = (2.0f * M_PI * RHO) * T;
    float phi = aperture_size * sqrtf(T/N);
    
    _set_tilt(theta, phi);
}
void Projector::_draw ()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLsizei W_ = static_cast<GLsizei>(W),
            h_ = static_cast<GLsizei>(h);

    float trail_time = 0.0f;
    if (trail) {
        if (trailing) trail->add_point(animator->theta, animator->time);
        if (not trail_paused) trail_time = animator->time;
    }

    if (in_stereo) {
        //right eye
        glViewport(0,0,W_,h_);
        mat_mult(tilt, animator->twist_theta(-TWIST_ANGLE), temp1);
        drawing->reproject(temp1);
        drawing->display();
        if (trail) trail->display(temp1, trail_time);

        //left eye
        glViewport(W_,0,W_,h_);
        mat_mult(tilt, animator->twist_theta(TWIST_ANGLE), temp1);
        drawing->reproject(temp1);
        drawing->display();
        if (trail) trail->display(temp1, trail_time);
    } else {
        glViewport(0,0,W_,h_);
        mat_mult(tilt, animator->theta, temp1);
        drawing->reproject(temp1);
        drawing->display();
        if (trail) trail->display(temp1, trail_time);
    }
}
void Projector::display (char* output)
{
    if (_update_needed) _update();

    if (not (paused and motion_blur)) {
        if (motion_blur)    _set_rand_tilt();
        else                _set_tilt(0,0);
        _draw();

        bool effects = motion_blur or reverse_colors or high_contrast;
        if (effects) { //slower, using accum buffer
            _draw_buffer();
            _show_buffer(output);
        } else { //faster, no accum buffer
#ifdef CAPTURE
            if (output) _capture_little(output);
#endif
            finish_buffer();
        }
    } else { //draw a long-exposure image

        //LATER this ignores color inversion, contrast, etc.
        const int N = NUM_STILL_FRAMES / (high_quality ? 1 : 4);
        const float part = 1.0f / N;
        glClear(GL_ACCUM_BUFFER_BIT);
        for (int n=0; n<N; ++n) {
            _set_unif_tilt(n,N);
            _draw();
            glAccum(GL_ACCUM, part);

            //watch image develop
            //logger.debug() << "  frame " << n+1 << " / " << N |0;
            glAccum(GL_RETURN, 1.0f);
            if (n == N-1) _show_buffer(output);
            else          _show_buffer();
        }
    }
}
#ifdef CAPTURE
void Projector::_capture_little (char* image)
{//capture buffer to little picture
    glPixelTransferf(GL_RED_BIAS,   0.0f);
    glPixelTransferf(GL_GREEN_BIAS, 0.0f);
    glPixelTransferf(GL_BLUE_BIAS,  0.0f);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    bool accumulating=(!in_color)&&(!high_quality);
    float scale=(!in_color)&&high_quality?0.33333333f:1.0f;
    glPixelTransferf(GL_RED_SCALE,scale);
    glPixelTransferf(GL_GREEN_SCALE,scale);
    glPixelTransferf(GL_BLUE_SCALE,scale);
    if(accumulating){
        glAccum(GL_LOAD, 1.0f);
        glAccum(GL_RETURN, 0.3333333f);
    };
    glFinish();
    glReadPixels(0,0,w,h, in_color?GL_RGB:GL_LUMINANCE, GL_UNSIGNED_BYTE, image);
    if(accumulating) glAccum(GL_RETURN, 1.0f);
}

void Projector::capture (unsigned Nwide, unsigned Nhigh)
{//captures, currently only grayscale
    logger.info() << "capturing " << Nwide << " x " << Nhigh << " screens in "
        << (in_color ? "color" : "grayscale") |0;
    Logging::IndentBlock block;

    //check to open file first
    logger.info() << "opening file: jenn_capture.png" |0;
    FILE *file = fopen("jenn_capture.png", "wb");
    if (not file) {
        logger.warning() << "couldn't open file for writing" |0;
        return;
    }

    //allocate little-picture memory
    logger.debug() << "allocating little picture" |0;
    size_t color_bytes = in_color ? 3 : 1;
    char* image = new(std::nothrow) char[(w+4) * (h+4) * color_bytes];
    if (image == NULL) {
        logger.warning() << "too little memory for little image" |0;
        return;
    }

    //allocate big-picture memory
    logger.debug() << "allocating big picture" |0;
    int w_tot = w * Nwide;
    int h_tot = h * Nhigh;
    char* image_tot = new(std::nothrow) char[w_tot * h_tot * color_bytes];
    if (image_tot == NULL) {
        logger.warning() << "too little memory for bit image" |0;
        delete[] image;
        return;
    }

    //calculate geometry
    logger.debug() << "geometry: " << w_tot << " x " << h_tot << " pixels" |0;
    Assert (paused, "must be paused to shoot");
    float scale_factor = 1.0f / max(Nwide, Nhigh);
    zoom(scale_factor);
    float x_center_tot = x_center;
    float y_center_tot = y_center;
    float x_offset = 2 * animator->vis_rad * w_factor;
    float y_offset = 2 * animator->vis_rad * h_factor;

    //capture screens
    drawing->set_clipping(false); //clipping math fails for tiled images
    logger.debug() << "capturing screens:" |0;
    for (unsigned i=0; i<Nwide; ++i) {
    for (unsigned j=0; j<Nhigh; ++j) {
        Logging::IndentBlock block;
        logger.info() << "screen " << i+1 << ", " << j+1 << "..." |0;
        //draw little image
        x_center = x_center_tot + x_offset * (i + 0.5f * (1.0f - Nwide));
        y_center = y_center_tot + y_offset * (j + 0.5f * (1.0f - Nhigh));
        display(image);
        glAccum(GL_RETURN, 1.0f);

        //copy to big picture
        char* source=image;
        int w_raw=color_bytes*w;
        int w_tot_raw=color_bytes*w_tot;
        char* dest=image_tot+w_tot_raw*h*j+w_raw*i;
        for (int y=0; y<h; ++y) {
            memcpy(dest, source, w_raw);
            source+=w_raw;
            dest+=w_tot_raw;
        }
    }}
    drawing->set_clipping(true); //clipping math fails for tiled images
    delete[] image;

    //clean up geometry
    logger.debug() << "restoring geometry" |0;
    x_center = x_center_tot;
    y_center = y_center_tot;
    zoom(1.0f/scale_factor);

    //write to png file
    logger.debug() << "writing png file:" |0;
    //  set up write structure
    logger.debug() << "  defining header" |0;
    png_structp p_writer = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                   NULL, NULL, NULL);
    png_infop p_info = png_create_info_struct(p_writer);
	png_init_io(p_writer, file);
    png_set_IHDR(p_writer, p_info,
                 w_tot, h_tot,
	    	     8,                     //bit depth
                 in_color ? PNG_COLOR_TYPE_RGB
                          : PNG_COLOR_TYPE_GRAY,
                 PNG_INTERLACE_NONE,
		         PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
	png_write_info(p_writer, p_info);

    //  write image row-by-row
    logger.debug() << "  writing data" |0;
    png_byte** rows = new png_byte*[h_tot];
    for (int y=0; y<h_tot; ++y) {
        rows[y] = (png_byte*)(image_tot + color_bytes
                                        * (w_tot * (h_tot -y -1)));
    }
	png_write_image(p_writer, rows);
    delete[] rows;
    delete[] image_tot;

    //  finish png file
    logger.debug() << "  finishing file" |0;
	png_write_end(p_writer, NULL);
    fclose(file);

    logger.info() << "finished capturing." |0;
}
#endif
int Projector::select (int X, int Y)
{
    if (in_stereo) { //select viewport
        if (X > W) { //left side
            X -= static_cast<int>(W);
            drawing->reproject(animator->twist_theta(TWIST_ANGLE));
        } else {
            drawing->reproject(animator->twist_theta(-TWIST_ANGLE));
        }
    }

    float x = convert_x(X);
    float y = convert_y(Y);
    return drawing->select(x,y);
}
void Projector::set_drawing (bool updating)
{
    drawing->set_quality(high_quality);
    animator->reset(true);
    float d_rad = drawing->get_radius();
    logger.debug() << "setting radius to " << d_rad |0;
    animator->set_radius0(d_rad * BORDER_RADIUS);
    x_center = y_center = 0.0f;
    if (updating) update();
}

bool Projector::pan (int X, int Y)
{
    if (!panning) return false;

    x_center -= animator->vis_rad * w_factor * 2.0 * (X - drag_X) / W;
    y_center += animator->vis_rad * h_factor * 2.0 * (Y - drag_Y) / h;

    drag_X = X;
    drag_Y = Y;

    update();
    return true;
}

}

