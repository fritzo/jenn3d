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

#ifndef JENN_MENUS_H
#define JENN_MENUS_H

#include "definitions.h"

#ifdef CYGWIN_HACKS
    #define GLUT_STATIC
#endif
#if defined(__APPLE__) && defined(__MACH__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <set>

namespace Menus
{

const Logging::Logger logger("menus", Logging::INFO);

//fonts
class Font
{
public:
    int height;
    void* font;
    float opacity;
    Font(int h, void* f, float o) : height(h), font(f), opacity(o) {}
};
//  the larger, the less opaque
const Font BM8x13 (13, GLUT_BITMAP_8_BY_13,        1.000f);
const Font BM9x15 (15, GLUT_BITMAP_9_BY_15,        1.000f);
const Font Times10(11, GLUT_BITMAP_TIMES_ROMAN_10, 1.000f);
const Font Times24(26, GLUT_BITMAP_TIMES_ROMAN_24, 0.500f);
const Font Helv10 (12, GLUT_BITMAP_HELVETICA_10,   1.000f);
const Font Helv12 (14, GLUT_BITMAP_HELVETICA_12,   1.000f);
const Font Helv18 (20, GLUT_BITMAP_HELVETICA_18,   0.666f);

//abstract menu
class Menu
{
//static
    typedef std::set<std::pair<int,Menu*> > Menus;
    typedef Menus::iterator miter;
    typedef Menus::reverse_iterator mriter;
    static Menus s_menus;
protected:
    static bool menus_hidden;
private:

//individual
    const bool managed; //either self-deletable or managed
protected:
    const char* m_string;
    const Font m_font;
    float x,y;
    int x0,y0,x1,y1;
    const int m_raised;
    static int s_raised;
    virtual ~Menu ();
public:
    Menu (const char* s, Font f=Helv18,
            float x=0.5, float y=0.5, bool _managed=true);

protected:
    virtual void _display ();
    virtual bool _mouse (int button, int X, int Y);
    virtual void _reshape (int w, int h);
    virtual void _call (int N) { if (managed) delete this; }
public:
    void close () { Assert(not managed, "can't close this menu"); delete this; }
    static void clear ();
    static void display ();
    static bool mouse (int button, int state, int x, int y);
    static void reshape (int w, int h);
};

//start message
void start_menu ();

//root menu
class RootMenu : public Menu
{
    static RootMenu* s_unique_instance;
protected:
    virtual void _call (int N);
    virtual ~RootMenu () { s_unique_instance = NULL; }
public:
    RootMenu ();
    static void open ();
};

//help menu
enum HelpType { MAIN_HELP, KEYBOARD_HELP };
class HelpMenu : public Menu
{
    static HelpMenu* s_unique_instance;
    static HelpType s_current_type;
protected:
    virtual void _call (int N);
    virtual ~HelpMenu () { s_unique_instance = NULL; }
public:
    HelpMenu (HelpType type);
    static void open (HelpType type = MAIN_HELP);
};
inline void main_help () { HelpMenu::open(MAIN_HELP); }
inline void keyboard_help () { HelpMenu::open(KEYBOARD_HELP); }

//capturing
#ifdef CAPTURE
class CaptureMenu : public Menu
{
    static CaptureMenu* s_unique_instance;
protected:
    virtual void _call (int N);
    virtual ~CaptureMenu () { s_unique_instance = NULL; }
public:
    CaptureMenu ();
    static void open ();
};
#endif

//exporting geometry
class ExportMenu : public Menu
{
    static ExportMenu* s_unique_instance;
protected:
    virtual void _call (int N);
    virtual ~ExportMenu () { s_unique_instance = NULL; }
public:
    ExportMenu ();
    static void open ();
};

//motion
class MotionMenu : public Menu
{
    static MotionMenu* s_unique_instance;
protected:
    virtual void _call (int N);
    virtual ~MotionMenu () { s_unique_instance = NULL; }
public:
    MotionMenu ();
    static void open ();
};

//flying
class FlyingMenu : public Menu
{
    static FlyingMenu* s_unique_instance;
protected:
    virtual void _call (int N);
    virtual ~FlyingMenu () { s_unique_instance = NULL; }
public:
    FlyingMenu ();
    static void open ();
};

//style
class StyleMenu : public Menu
{
    static StyleMenu* s_unique_instance;
protected:
    virtual void _call (int N);
    virtual ~StyleMenu () { s_unique_instance = NULL; }
public:
    StyleMenu ();
    static void open ();
};

//camera
class CameraMenu : public Menu
{
    static CameraMenu* s_unique_instance;
protected:
    virtual void _call (int N);
    virtual ~CameraMenu () { s_unique_instance = NULL; }
public:
    CameraMenu ();
    static void open ();
};

//view
class ViewMenu : public Menu
{
    static ViewMenu* s_unique_instance;
protected:
    virtual void _call (int N);
    virtual ~ViewMenu () { s_unique_instance = NULL; }
public:
    ViewMenu ();
    static void open ();
};

//families of polytopes
class FamilyMenu : public Menu
{
    static FamilyMenu* s_unique_instance;
protected:
    virtual void _call (int N);
    virtual ~FamilyMenu ();
public:
    FamilyMenu ();
    static void open ();
};

//individual polytopes
class ModelMenu : public Menu
{
    static ModelMenu* s_unique_instance;
    const int m_size;
    const int *m_nums, *m_edges, *m_faces, *m_weights;
protected:
    virtual void _call (int N);
    virtual ~ModelMenu () { s_unique_instance = NULL; }
    ModelMenu (const char* message, int size,
               const int* nums, const int* edges=NULL, const int* faces=NULL,
               const int* weights=NULL);
public:
    static void open (const char* message, int size, const int* nums);
    static void open (const char* message, int size,
                      const int* nums, const int* edges, const int* faces,
                      const int* weights);
    static void open_fam (const char* prefix, int size, const int* nums);
    static void close ();
};

}

#endif

