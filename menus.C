/*
This file is part of Jenn.
Copyright 2001-2010 Fritz Obermeyer.

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

#include "menus.h"
#include "main.h"
#include <vector>
#include <cstring>

#ifdef __EMSCRIPTEN__
#include "emscripten/bind.h"
#endif

namespace Menus
{

#define MENU_DEPTH 1.1f
#define TEXT_DEPTH 1.2f

#define SCREEN_BORDER 0.01f
#define MENU_HBORDER 3
#define MENU_VBORDER 2

//[ ghetto menuing system ]--------------------------
Menu::Menus Menu::s_menus;
int Menu::s_raised = 0;
bool Menu::menus_hidden = false;
Menu::Menu (const char* s, Font f, float _x, float _y, bool _managed)
    : managed(_managed), m_string(s), m_font(f),
      x(_x), y(_y), x0(0), y0(0), m_raised(s_raised++)
{
    s_menus.insert(std::make_pair(m_raised, this));
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    _reshape(w,h);
}
Menu::~Menu () { s_menus.erase(std::make_pair(m_raised, this)); }
void bitmapString (const char *s, int x, int y, Font font)
{
    y += 4 - font.height;
    glRasterPos2i(x, y);
    for (unsigned i=0; i<strlen(s); ++i) {
        char c = s[i];
        if (c == '\n') {
            y -= font.height;
            glRasterPos2i(x, y);
        } else {
            glutBitmapCharacter(font.font, c);
        }
    }
}
void Menu::clear ()
{
    for (miter i=s_menus.begin(); i!=s_menus.end(); ++i) {
        if (i->second->managed) delete i->second;
    }
    s_raised = 1;
}
void Menu::display ()
{
#ifndef __EMSCRIPTEN__
    if (menus_hidden) return;
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    glViewport(0,0,w,h);
    for (miter i=s_menus.begin(); i!=s_menus.end(); ++i) {
        i->second->_display();
    }
#endif
}
Menu* g_start_menu = NULL;
bool Menu::mouse (int button, int state, int X, int Y)
{// returns whether a menu was found
    if (g_start_menu) {
        delete g_start_menu;
        g_start_menu = NULL;
    }
    if (state != GLUT_DOWN) return false;
    int x = X, y = glutGet(GLUT_WINDOW_HEIGHT) - Y;
    for (mriter i=s_menus.rbegin(); i!=s_menus.rend(); ++i) {
        if (i->second->_mouse(button, x, y)) return true;
    }
    return false;
}
void Menu::reshape (int w, int h)
{
#ifndef __EMSCRIPTEN__
    for (miter i=s_menus.begin(); i!=s_menus.end(); ++i) {
        i->second->_reshape(w,h);
    }
#endif
}
void Menu::_display ()
{
    //set up window
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, 0, h, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //draw background
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glColor4f(COLOR_BG,0.75f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBegin(GL_QUADS);
    glVertex2f(x0-MENU_HBORDER, y0-MENU_VBORDER);
    glVertex2f(x0-MENU_HBORDER, y1+MENU_VBORDER);
    glVertex2f(x1+MENU_HBORDER, y1+MENU_VBORDER);
    glVertex2f(x1+MENU_HBORDER, y0-MENU_VBORDER);
    glEnd();

    //draw text
    glColor4f(COLOR_FG, m_font.opacity);
    bitmapString(m_string, x0, y1, m_font);
    glEnable(GL_DEPTH_TEST);
}
bool Menu::_mouse (int button, int x, int y)
{
    if ((x < x0) or (x1 < x) or (y < y0) or (y1 < y)) return false;
    if (menus_hidden) {
        menus_hidden = false;
    } else {
        switch (button) {
            case GLUT_LEFT_BUTTON:
                _call((y1 - y) / m_font.height);
                break;
            default:
                if (managed) delete this;
        }
    }
    return true;
}
void Menu::_reshape (int w, int h)
{
    x0 = 0;
    y0 = 0;

    //measure text
    int dx=0, dxi=0, dy=m_font.height;
    for (unsigned i=0; i<strlen(m_string); ++i) {
        char c = m_string[i];
        if (c == '\n') {
            dx = max(dx,dxi);
            dxi = 0;
            dy += m_font.height;
        } else {
            dxi += glutBitmapWidth(m_font.font, c);
        }
    }
    dx = max(dx,dxi);

    //set bounds
    x1 = x0 + dx;
    y1 = y0 + dy;

    //shift to (x,y)
    dx = int((w-dx) * x);
    dy = int((h-dy) * y);
    x0 += dx; x1 += dx;
    y0 += dy; y1 += dy;
}

//start menu
const char * const start_message =
"                         Jenn3d version 2010:02:27\n\
              copyright 2001-2010 Fritz Obermeyer.\n\
Jenn3d is released under version 2 of the GNU GPL.\n\n\
             Click on `help' to start help system.\n\n\
  any command-line arguments suppress this message";
void start_menu ()
{
#ifndef __EMSCRIPTEN__
    Assert (!g_start_menu, "tried to create two start menus")
    g_start_menu = new Menu(start_message);
#endif
}

//[ help dialogs ]---------------------
enum HelpDialogs
{
    H_MODEL,
    H_VIEW,
    H_MOVE, H_MOUSE, H_SPRING,
    H_FLYING,
    H_STYLE,
    H_CAMERA,
    H_EXPORT,
    H_SAVE,
    NUM_HELP_DLGS
};
std::vector<bool> show_help(NUM_HELP_DLGS, false);
void start_help ()
{
    for (unsigned i=0; i<NUM_HELP_DLGS; ++i) {
        show_help[i] = true;
    }
}

//[ help ]---------------------
const char * const main_help_message = 
"tips for using Jenn-\n\
   - all three mouse buttons control motion\n\
     (if you don't have three buttons, try flipping the mouse (move menu)\n\
   - right click on a window to close it\n\
   - if models look crappy, try toggling the quality (camera menu)\n\
   - try viewing large models in line-art style\n\
       or with one or two of vertices/edges/faces hidden (it's faster)\n\
   - hit the 'k' key for a list of keyboard shortcuts\n\
for more information visit   www.jenn3d.org";
const char * const keyboard_help_message = 
"                        Keyboard Shortcuts\n\
    k  -  prints this message\n\
    h  -  prints help message\n\
    ESCAPE  -  exits Jenn\n\
    d  -  toggles drawing mode\n\
model\n\
    F1-F12  -  selects a basic model\n\
    SHIFT + F1-F12  -  select a complicated model\n\
view\n\
    UP/DOWN  -  zooms in and out\n\
    1/2  -  selects mono/stereo\n\
    SHIFT+ARROW  -  pans image a little\n\
    CTRL+ARROW  -  pans image one screen\n\
    P  -  pans to center\n\
    F  -  sets fullscreen\n\
move\n\
    SPACEBAR/D  -  toggles brakes/drag\n\
    m  -  swaps mouse axes (for 1- or 2-button mice)\n\
    c/C  -  toggles spring/sets spring's center\n\
    s/S  -  slows down/Speeds up\n\
    x  -  resets motion\n\
flying\n\
    y  -  toggles flying mode\n\
    t  -  toggles ribbon trail\n\
    T  -  breaks trail\n\
style\n\
    v/e/f  -  toggles vertices/edges/faces\n\
    +/-  -  thickens/thins edges\n\
    w/l/h  -  toggles wireframe / line-art / haze\n\
camera\n\
    q  -  toggles high/low quality\n\
    b  -  toggles motion+depth blurring\n\
    n/N  -  focuses lens nearer/farther\n\
    a/A  -  shrinks/widens aperture\n\
    K  -  toggles high-contrast\n\
    r  -  reverses colors\n\
    p  -  pauses (shoots picture)\n\
    X  -  resets lens";
const char* help_messages[2] = {
    main_help_message,
    keyboard_help_message
};
const Font help_fonts[2] = {
    Helv18,
    Helv12
};
    
HelpMenu* HelpMenu::s_unique_instance = NULL;
HelpType HelpMenu::s_current_type = MAIN_HELP;
HelpMenu::HelpMenu (HelpType type)
    : Menu(help_messages[type], help_fonts[type], 0.5f, 0.5f)
{
    Assert(s_unique_instance==NULL, "extra HelpMenu");
    s_unique_instance = this;
    s_current_type = type;
}
void HelpMenu::open (HelpType type)
{
    if (HelpMenu::s_unique_instance) {
        delete HelpMenu::s_unique_instance;
        if (type == HelpMenu::s_current_type) return;
    }

    new HelpMenu(type);
    start_help();
}
void HelpMenu::_call (int N) { delete this; }

#ifdef CAPTURE
//[ capturing ]---------------------
const char * const capt_message = 
#if (CAPTURE > 4)
"toggle quality\n\
grayscale\n\
color\n\
save image of size...\n\
    1 x 1    =     1 tile\n\
    2 x 2    =     4 tiles\n\
    3 x 3    =     9 tiles\n\
    4 x 4    =   16 tiles\n\
    6 x 6    =   36 tiles\n\
    8 x 8    =   64 tiles\n\
  12 x 12  = 144 tiles\n\
  16 x 16  = 256 tiles\n\
  24 x 24  = 576 tiles";
#else
"toggle quality\n\
grayscale\n\
color\n\
save image of size...\n\
  1 x 1    =     1 tile\n\
  2 x 2    =     4 tiles\n\
  3 x 3    =     9 tiles\n\
  4 x 4    =   16 tiles";
#endif
const char * const capt_help =
"Saving Images:\n\
  to capture a screen shot, click on \"1x1\".\n\
  to capture larger images, you can capture NxN screens.\n\
  images are written to a file named jenn_capture.png\n\
  Warning: many screens means bigger image files!";

CaptureMenu* CaptureMenu::s_unique_instance = NULL;
CaptureMenu::CaptureMenu ()
    : Menu(capt_message, Helv18, 1-SCREEN_BORDER, 1-SCREEN_BORDER)
{
    Assert(s_unique_instance==NULL, "extra CaptureMenu");
    s_unique_instance = this;
}
void CaptureMenu::open ()
{
    if (CaptureMenu::s_unique_instance) {
        delete CaptureMenu::s_unique_instance;
    } else {
        new CaptureMenu();
        if (show_help[H_SAVE]) { show_help[H_SAVE] = false;
            new Menu(capt_help);
        }
    }
}
void capture (int Nwide, int Nhigh)
{
    beg_pause(false);
    projector->capture(Nwide, Nhigh);
    end_pause();
}
void CaptureMenu::_call (int N)
{
    logger.debug() << "selected choice "  << N |0;

    switch (N) {
        case 0: projector->toggle_quality();    return;
        case 1: projector->set_color(false);    return;
        case 2: projector->set_color(true);     return;
        case 3: return;
        default: break;
    }

    //draw waiting dialog
    const char* message = projector->get_quality()
        ? "saving high-quality image to:\n    jenn_capture.png\n(this may take a while)"
        : "saving low-quality image to:\n    jenn_capture.png";
    Menu* dialog = new Menu(message, Helv18, 0.5, 0.5, false);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    finish_buffer();

    switch (N) {
        case 4: capture(1,1);           break;
        case 5: capture(2,2);           break;
        case 6: capture(3,3);           break;
        case 7: capture(4,4);           break;
#if (CAPTURE > 4)
        case 8: capture(6,6);           break;
        case 9: capture(8,8);          break;
        case 10: capture(12,12);        break;
        case 11: capture(16,16);        break;
        case 12: capture(24,24);        break;
#endif
    }
    dialog->close();
}
#endif

//[ exporting ]---------------------
const char * const exp_message = 
"export to STL\n\
toggle quality\n\
+ min thickness\n\
- min thickness\n\
export graph";
const char * const exp_help =
"Exporting Geometry:\n\
  export to STL -- exports geometry to the file jenn_export.stl\n\
  toggle quality -- toggles between coarse/fine meshes\n\
  +/- thickness --  adjusts min edge/vertex thickness\n\
  Warning: exported file does not pedantically follow stl standard\n\
  export graph -- exports graph to simple text file";

ExportMenu* ExportMenu::s_unique_instance = NULL;
ExportMenu::ExportMenu ()
    : Menu(exp_message, Helv18, 1-SCREEN_BORDER, 1-SCREEN_BORDER)
{
    Assert(s_unique_instance==NULL, "extra ExportMenu");
    s_unique_instance = this;
}
void ExportMenu::open ()
{
    if (ExportMenu::s_unique_instance) {
        delete ExportMenu::s_unique_instance;
    } else {
        new ExportMenu();
        if (show_help[H_SAVE]) { show_help[H_SAVE] = false;
            new Menu(exp_help);
        }
    }
}
void ExportMenu::_call (int N)
{
    logger.debug() << "selected choice "  << N |0;

    const char* message = "exporting geometry to jenn_export.png";

    switch (N) {
#ifdef __EMSCRIPTEN__
        case 0: drawing->export_stl();                              break;
#else
        case 0: {
            //draw waiting dialog
            Menu* dialog = new Menu(message, Helv18, 0.5, 0.5, false);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            finish_buffer();

            //export
            drawing->export_stl();

            //erase dialog
            dialog->close();
            return;
        }
#endif
        case 1: projector->toggle_quality();                        break;
        case 2: drawing->set_coating(drawing->get_coating()*1.4f);  break;
        case 3: drawing->set_coating(drawing->get_coating()/1.4f);  break;
        case 4: drawing->export_graph();                            break;
        default: break;
    }
}

//motion menu
const char * const motion_message = 
"flip mouse\n\
drag on/off\n\
hi/lo drag\n\
springy\n\
flip spring\n\
tare spring\n\
slower\n\
faster\n\
reset\n\
pause";
const char * const motion_help =
"Motion:\n\
  if you have a 1- or 2- button mouse, you can switch mouse channels\n\
      by selecting \"flip mouse\"\n\
  toggle the high/low drag to let the model float around freely\n\
  to make the model spring to the center, select \"springy\"\n\
      you can also invert the spring by selecting \"flip\", or\n\
      set the springs center with \"tare\" (useful for capturing)\n\
  the if things are moving too slow or fast, adjust jenn's speed\n\
  if jenn is going crazy, try selecting \"reset\"";
MotionMenu* MotionMenu::s_unique_instance = NULL;
MotionMenu::MotionMenu ()
    : Menu(motion_message, Helv18, SCREEN_BORDER, SCREEN_BORDER)
{
    Assert(s_unique_instance==NULL, "extra MotionMenu");
    s_unique_instance = this;
}
void MotionMenu::open ()
{
    if (MotionMenu::s_unique_instance) {
        delete MotionMenu::s_unique_instance;
    } else {
        new MotionMenu();
        if (show_help[H_MOVE]) { show_help[H_MOVE] = false;
            new Menu(motion_help);
        }
    }
}
void MotionMenu::_call (int N)
{
    logger.debug() << "selected choice "  << N |0;
    switch (N) {
        case 0: animator->shift_channels();        break;
        case 1: animator->toggle_drifting();       break;
        case 2: animator->toggle_stopped();        break;
        case 3: animator->toggle_centered();       break;
        case 4: animator->invert();                break;
        case 5: animator->set_center();            break;
        case 6: animator->slow_down();             break;
        case 7: animator->speed_up();              break;
        case 8: animator->reset();
                projector->pan_center();               break;
        case 9: toggle_pause();                   break;
        default: delete this;
    }
}

//[ flying ]---------------------
//flying menu
const char * const flying_message = 
"flying\n\
forward\n\
backward\n\
trail line\n\
trail ribbon\n\
trail tube\n\
break trail\n\
pause trail\n\
hide model";
const char * const flying_help =
"Flying:\n\
  to fly through a model, select \"flying\"\n\
      (your initial speed is higher the longer you hold down)\n\
  when flying:\n\
      use the left mouse button to steer\n\
      use the right mouse button to control speed and roll\n\
      try zooming in or zooming out to make things look right\n\
  to accelerate forward or backward, select \"forward\" or \"backward\"\n\
  to start leaving a trail, select  \"trailing\"\n\
  to break a trail or start a new trail, select \"break trail\"";
FlyingMenu* FlyingMenu::s_unique_instance = NULL;
FlyingMenu::FlyingMenu ()
    : Menu(flying_message, Helv18, 1.0f-SCREEN_BORDER, 1.0f-SCREEN_BORDER)
{
    Assert(s_unique_instance==NULL, "extra FlyingMenu");
    s_unique_instance = this;
}
void FlyingMenu::open ()
{
    if (FlyingMenu::s_unique_instance) {
        delete FlyingMenu::s_unique_instance;
    } else {
        new FlyingMenu();
        if (show_help[H_FLYING]) { show_help[H_FLYING] = false;
            new Menu(flying_help);
        }
    }
}
void FlyingMenu::_call (int N)
{
    logger.debug() << "selected choice "  << N |0;
    switch (N) {
        case 0: animator->toggle_flying();                  break;
        case 1: animator->accel_fwd( 0.1f);                 break;
        case 2: animator->accel_fwd(-0.1f);                 break;
        case 3: projector->toggle_trail(Trails::LINE);      break;
        case 4: projector->toggle_trail(Trails::RIBBON);    break;
        case 5: projector->toggle_trail(Trails::TUBE);      break;
        case 6: projector->toggle_trailing();               break;
        case 7: projector->pause_trail();                   break;
        case 8: drawing->toggle_edges();
                drawing->toggle_verts();
                drawing->toggle_faces();                    break;
        default: delete this;
    }
}

//[ style ]---------------------
const char * const style_message = 
"faces\n\
edges\n\
vertices\n\
line-art\n\
curved\n\
wireframe\n\
thinner\n\
fatter\n\
fog";
StyleMenu* StyleMenu::s_unique_instance = NULL;
StyleMenu::StyleMenu ()
    : Menu(style_message, Helv18, 1-SCREEN_BORDER, 0.5f)
{
    Assert(s_unique_instance==NULL, "extra StyleMenu");
    s_unique_instance = this;
}
void StyleMenu::open ()
{
    if (StyleMenu::s_unique_instance) {
        delete StyleMenu::s_unique_instance;
    } else {
        new StyleMenu();
    }
}
void StyleMenu::_call (int N)
{
    logger.debug() << "selected choice "  << N |0;
    switch (N) {
        case 0: drawing->toggle_faces();                             break;
        case 1: drawing->toggle_edges();                             break;
        case 2: drawing->toggle_verts();                             break;
        case 3: drawing->toggle_fancy();                             break;
        case 4: drawing->toggle_curved();                            break;
        case 5: projector->toggle_wireframe();                       break;
        case 6: drawing->set_tube_rad(drawing->get_tube_rad()/1.2f); break;
        case 7: drawing->set_tube_rad(drawing->get_tube_rad()*1.2f); break;
        case 8: drawing->toggle_hazy();                              break;
        default: delete this;
    }
}

//[ camera ]---------------------
const char * const camera_message = 
"hi/lo quality\n\
hi/lo contrast\n\
invert colors\n\
motion blur\n\
+ aperture\n\
- aperture\n\
focus nearer\n\
focus farther\n\
reset lens\n\
shoot";
CameraMenu* CameraMenu::s_unique_instance = NULL;
CameraMenu::CameraMenu ()
    : Menu(camera_message, Helv18, 1-SCREEN_BORDER, SCREEN_BORDER)
{
    Assert(s_unique_instance==NULL, "extra CameraMenu");
    s_unique_instance = this;
}
void CameraMenu::open ()
{
    if (CameraMenu::s_unique_instance) {
        delete CameraMenu::s_unique_instance;
    } else {
        new CameraMenu();
    }
}
void CameraMenu::_call (int N)
{
    logger.debug() << "selected choice "  << N |0;
    switch (N) {
        case 0: projector->toggle_quality();            break;
        case 1: projector->toggle_contrast();           break;
        case 2: projector->toggle_reversed();           break;
        case 3: projector->toggle_blur();               break;
        case 4: projector->scale_aperture(M_SQRT2);     break;
        case 5: projector->scale_aperture(1/M_SQRT2);   break;
        case 6: projector->move_nearer();               break;
        case 7: projector->move_farther();              break;
        case 8: projector->reset();                     break;
        case 9: beg_pause();                            break;
        default: delete this;
    }
}

//[ navigation ]---------------------
const char * const view_message = 
"stereo/mono\n\
zoom out\n\
zoom in\n\
pan left\n\
pan right\n\
pan up\n\
pan down\n\
pan center\n\
fullscreen";
ViewMenu* ViewMenu::s_unique_instance = NULL;
ViewMenu::ViewMenu ()
    : Menu(view_message, Helv18, SCREEN_BORDER, 0.5f)
{
    Assert(s_unique_instance==NULL, "extra ViewMenu");
    s_unique_instance = this;
}
void ViewMenu::open ()
{
    if (ViewMenu::s_unique_instance) {
        delete ViewMenu::s_unique_instance;
    } else {
        new ViewMenu();
    }
}
void ViewMenu::_call (int N)
{
    logger.debug() << "selected choice "  << N |0;
    switch (N) {
        case 0: projector->toggle_stereo();     break;
        case 1: projector->zoom(ZOOM_OUT);      break;
        case 2: projector->zoom(ZOOM_IN);       break;
        case 3: projector->pan_left (0.0625f);  break;
        case 4: projector->pan_right(0.0625f);  break;
        case 5: projector->pan_up   (0.0625f);  break;
        case 6: projector->pan_down (0.0625f);  break;
        case 7: projector->pan_center();        break;
        case 8: toggle_fullscreen();            break;
        default: delete this;
    }
}

//[ families ]---------------------
//model selection menu
const char * const model_message = 
"polyhedra\n\
polychora\n\
duoprisms\n\
truncated `hedra\n\
truncated `chora\n\
bitrunc. `chora\n\
edge-t. `hedra\n\
edge-t. `chora\n\
cayley graphs\n\
solids\n\
mazes\n\
222-family\n\
227-family\n\
233-family\n\
234-family\n\
235-family\n\
  Y-family\n\
333-family\n\
334-family\n\
343-family\n\
335-family";
const char * const model_help =
"Models:\n\
  select a family and then a model to view\n\
  larger models are usually lower on the list\n\
  very large models may look better in line-art style\n\
  my favorite is the bitruncated 120-cell = 335-fam 1,4";
FamilyMenu* FamilyMenu::s_unique_instance = NULL;
FamilyMenu::FamilyMenu ()
    : Menu(model_message, Helv18, 0.15f, 1-SCREEN_BORDER)
{
    Assert(s_unique_instance==NULL, "extra FamilyMenu");
    s_unique_instance = this;
}
void FamilyMenu::open ()
{
    if (FamilyMenu::s_unique_instance) {
        delete FamilyMenu::s_unique_instance;
    } else {
        new FamilyMenu();
        if (show_help[H_MODEL]) { show_help[H_MODEL] = false;
            new Menu(model_help);
        }
    }
}

//[ models ]---------------------
ModelMenu* ModelMenu::s_unique_instance = NULL;
ModelMenu::ModelMenu (const char* message, int size,
                      const int* nums, const int* edges, const int* faces,
                      const int* weights)
    : Menu(message, Helv18, 0.35f, 1-SCREEN_BORDER),
      m_size(size),
      m_nums(nums), m_edges(edges), m_faces(faces), m_weights(weights)
{
    Assert(s_unique_instance==NULL, "extra ModelMenu");
    s_unique_instance = this;
}
void ModelMenu::close ()
{
    if (ModelMenu::s_unique_instance) {
        delete ModelMenu::s_unique_instance;
    }
}
FamilyMenu::~FamilyMenu ()
{
    s_unique_instance = NULL;
    ModelMenu::close();
}
void ModelMenu::_call (int N)
{
    if (0 <= N and N < m_size) {
#ifndef __EMSCRIPTEN__
        //open waiting dialog
        Menu* dialog = new Menu("building model...", Helv18, 0.5, 0.5, false);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        finish_buffer();
#endif

        //build model
        int edges = m_edges ? m_edges[N] : 1111;
        int faces = m_faces ? m_faces[N] : 111111;
        int weights = m_weights ? m_weights[N] : 1111;
        Polytope::select(m_nums[N], edges, faces, weights);
        update_title();

#ifndef __EMSCRIPTEN__
        //close dialog
        dialog->close();
#endif
    }
}

//misc full-faced families
void ModelMenu::open (const char* message, int size, const int* nums)
{
    if (ModelMenu::s_unique_instance) {
        delete ModelMenu::s_unique_instance;
    }
    new ModelMenu(message, size, nums);
}
const char* phedra_message =
    "tetrahedron\ncube\noctahedron\ndodecahedron\nicosahedron";
const int phedra_nums[5] = {
    322322234, 422322234, 322422234, 522322234, 322522234};
const char* pchora_message = 
    "5-cell\n8-cell\n16-cell\n24-cell\n120-cell\n600-cell";
const int pchora_nums[6] = {
    322323234, 422323234, 322324234, 322423234, 522323234, 322325234};
const char* dprism_message = 
    "  3x3\n  4x4\n  4x5\n  4x6\n  5x5\n  4x10\n12x12\n18x18";
const int dprism_nums[8] = {
    322223014,222222000,222225004,222223000,
    522225014,222225000,622226000,922229000};
const char* thedra_message = 
    "t. tetra.\nt. cube\nt. octa.\nt. dodeca.\nt. icosa.";
const int thedra_nums[5] = {
    322322034,422322034,322422034,322522034,522322034};
const char* tchora_message = 
    "t. 5-cell\nt. 16-cell\nt. 8-cell\nt. 24-cell\nt. 120-cell\nt. 600-cell";
const int tchora_nums[6] = {
    422323034,322323034,322324034,322423034,522323034,322325034};
const char* bchora_message = 
    "bt.. 5-cell\nbt. 8-cell\nbt. 24-cell\nbt. 120-cell";
const int bchora_nums[4] = {
    322323014,422323014,322423014,522323014};
const char* ehedra_message = 
    "e.t. tetra.\ne.t. octa.\ne.t. cube\ne.t. dodeca.\ne.t. icosa.";
const int ehedra_nums[5] = {
    322322024,422322024,322422024,322522024,522322024};
const char* echora_message = 
    "e.t. 5-cell\ne.t. 8-cell\ne.t. 16-cell\ne.t. 24-cell\ne.t. 120-cell\ne.t. 600-cell";
const int echora_nums[6] = {
    322323024,422323024,322324024,322423024,522323024,322325024};
const char* cayley_message = 
    "2-2-2\n2-2-3\n2-2-5\n2-2-9\n2-3-3\n2-3-4\n2-3-5\n3-3-3\n    Y\n3-3-4\n3-4-3\n3-3-5";
const int cayley_nums[12] = {
    222222000,322222000,522222000,922222000,322322000,422322000,
    522322000,322323000,333222000,422323000,322423000,522323000};
const char* fam_message = 
    "X-X-X\n    1\n    2\n    3\n    4\n  1,2\n  1,3\n  1,4\n  2,3\n  2,4\n  3,4\n1,2,3\n1,2,4\n1,3,4\n2,3,4";

//quotient lattice families
void ModelMenu::open_fam (const char* prefix, int size, const int* nums)
{
    static char _fam_message[128];
    strcpy(_fam_message, fam_message);
    memcpy(_fam_message,prefix,5);

    if (ModelMenu::s_unique_instance) {
        delete ModelMenu::s_unique_instance;
    }
    new ModelMenu(_fam_message, size, nums);
}
const int fam222_nums[15] = {
    222222000,222222001,222222002,222222003,222222004,
    222222012,222222013,222222014,222222023,222222024,
    222222034,222222123,222222124,222222134,722222234};
const int fam227_nums[15] = {
    722222000,722222001,722222002,722222003,722222004,
    722222012,722222013,722222014,722222023,722222024,
    722222034,722222123,722222124,722222134,722222234};
const int fam233_nums[15] = {
    322322000,322322001,322322002,322322003,322322004,
    322322012,322322013,322322014,322322023,322322024,
    322322034,322322123,322322124,322322134,322322234};
const int fam234_nums[15] = {
    422322000,422322001,422322002,422322003,422322004,
    422322012,422322013,422322014,422322023,422322024,
    422322034,422322123,422322124,422322134,422322234};
const int fam235_nums[15] = {
    522322000, 522322001, 522322002, 522322003, 522322004,
    522322012, 522322013, 522322014, 522322023, 522322024,
    522322034, 522322123, 522322124, 522322134, 522322234};
const int fam333_nums[15] = {
    322323000, 322323001, 322323002, 322323003, 322323004,
    322323012, 322323013, 322323014, 322323023, 322323024,
    322323034, 322323234, 322323124, 322323134, 322323234};
const int fam_Y__nums[15] = {
    333222000, 333222001, 333222002, 333222003, 333222004,
    333222012, 333222013, 333222014, 333222023, 333222024,
    333222034, 333222124, 333222124, 333222134, 333222234};
const int fam334_nums[15] = {
    422323000, 422323001, 422323002, 422323003, 422323004,
    422323012, 422323013, 422323014, 422323023, 422323024,
    422323034, 322324234, 422323124, 422323134, 422323234};
const int fam343_nums[15] = {
    322423000, 322423001, 322423002, 322423003, 322423004,
    322423012, 322423013, 322423014, 322423023, 322423024,
    322423034, 322423234, 322423124, 322423134, 322423234};
const int fam335_nums[15] = {
    522323000, 522323001, 522323002, 522323003, 522323004,
    522323012, 522323013, 522323014, 522323023, 522323024,
    522323034, 322325234, 522323124, 522323134, 522323234};

//partial models (some edges & faces missing)
void ModelMenu::open (const char* message, int size,
        const int* nums, const int* edges, const int* faces, const int* weights)
{
    if (ModelMenu::s_unique_instance) {
        delete ModelMenu::s_unique_instance;
    }
    new ModelMenu(message, size, nums, edges, faces, weights);
}
const char* solids_message =
    "torus\ncubes\n8-hedra\n12-hedra\n20-hedra\n4-hedra\n6-prisms\n8-prisms";
const int solids_nums[8] = {
    722227000, 422323023, 322324034, 522323023,
    322325034, 522323034, 322423000, 422323000};
//in the following, '8' is only a placeholder in 8xxxx
const int solids_edges[8] = {
    81111, 80001, 80010, 80001, 80010, 80010, 81011, 81011};
const int solids_faces[8] = {
    8011110, 8000001, 8001000, 8000001, 8001000, 8001000, 8010101, 8010101};
const int solids_weights[8] = {1111, 1111, 1111, 1222, 2333, 1111, 1111, 1111};
const char* mazes_message = "{3,5}\n{3,3,3}\n{3,3,4}\n{3,4,3}\n{3,3,5}a\n{3,3,5}b\n{3,3,5}c\n{3,3,5}d";
const int mazes_nums[8] = {
    322522000, 322323000, 422323000, 322423000,
    522323013, 522323004, 522323003, 522323000};
const int mazes_edges[8] = {1111, 1111, 1111, 1111, 1111, 1111, 1111, 1111};
const int mazes_faces[8] = {
    8110011, 8011110, 8011110, 8110011, 8011000, 8011110, 8011110, 8011110};
const int mazes_weights[8] = {1111, 1111, 1111, 1111, 2111, 1111, 1111, 2111};

void FamilyMenu::_call (int N)
{
    logger.debug() << "selected choice "  << N |0;
    switch (N) {
        //misc full-faced families
        case  0: ModelMenu::open(phedra_message,  5, phedra_nums);  break;
        case  1: ModelMenu::open(pchora_message,  6, pchora_nums);  break;
        case  2: ModelMenu::open(dprism_message,  8, dprism_nums);  break;
        case  3: ModelMenu::open(thedra_message,  6, thedra_nums);  break;
        case  4: ModelMenu::open(tchora_message,  8, tchora_nums);  break;
        case  5: ModelMenu::open(bchora_message,  5, bchora_nums);  break;
        case  6: ModelMenu::open(ehedra_message,  6, ehedra_nums);  break;
        case  7: ModelMenu::open(echora_message,  8, echora_nums);  break;
        case  8: ModelMenu::open(cayley_message, 12, cayley_nums);  break;

        //partial-faced families
        case  9: ModelMenu::open(solids_message,  8, solids_nums,
                 solids_edges, solids_faces, solids_weights);       break;
        case 10: ModelMenu::open(mazes_message,   8, mazes_nums,
                 mazes_edges, mazes_faces, mazes_weights);          break;

        //quotient lattice families
        case 11: ModelMenu::open_fam("2-2-2",    15, fam222_nums);  break;
        case 12: ModelMenu::open_fam("2-2-7",    15, fam227_nums);  break;
        case 13: ModelMenu::open_fam("2-3-3",    15, fam233_nums);  break;
        case 14: ModelMenu::open_fam("2-3-4",    15, fam234_nums);  break;
        case 15: ModelMenu::open_fam("2-3-5",    15, fam235_nums);  break;
        case 16: ModelMenu::open_fam("    Y",    15, fam_Y__nums);  break;
        case 17: ModelMenu::open_fam("3-3-3",    15, fam333_nums);  break;
        case 18: ModelMenu::open_fam("3-3-4",    15, fam334_nums);  break;
        case 19: ModelMenu::open_fam("3-4-3",    15, fam343_nums);  break;
        case 20: ModelMenu::open_fam("3-3-5",    15, fam335_nums);  break;
    }
}

//[ root menu ]---------------------
const char * const root_message = 
#ifdef CAPTURE
"help\n\
model\n\
view\n\
move\n\
fly\n\
style\n\
camera\n\
save\n\
export\n\
hide\n\
exit";
#else
"help\n\
model\n\
view\n\
move\n\
fly\n\
style\n\
camera\n\
export\n\
hide\n\
exit";
#endif
RootMenu* RootMenu::s_unique_instance = NULL;
RootMenu::RootMenu ()
    : Menu(root_message, Helv18, SCREEN_BORDER, 1-SCREEN_BORDER, false)
{
    Assert(s_unique_instance==NULL, "extra RootMenu");
    s_unique_instance = this;
}
void RootMenu::open ()
{
    if (not RootMenu::s_unique_instance) new RootMenu();
}
void RootMenu::_call (int N)
{
    logger.debug() << "selected choice " << N |0;
    switch (N) {
        case 0: HelpMenu::open();       break;
        case 1: FamilyMenu::open();     break;
        case 2: ViewMenu::open();       break;
        case 3: MotionMenu::open();     break;
        case 4: FlyingMenu::open();     break;
        case 5: StyleMenu::open();      break;
        case 6: CameraMenu::open();     break;
#ifdef CAPTURE
        case 7: CaptureMenu::open();    break;
        case 8: ExportMenu::open();     break;
        case 9: menus_hidden = true;    break;
        case 10: exit(0);               break;
#else
        case 7: ExportMenu::open();     break;
        case 8: menus_hidden = true;    break;
        case 9: exit(0);                break;
#endif
    }
}

#ifdef __EMSCRIPTEN__
void select(int i, int j, int k) {
  static Menu* root_menu = Menu::s_menus.begin()->second;
  root_menu->_call(i);

  Menu* sub_menu = Menu::s_menus.rbegin()->second;
  sub_menu->_call(j);

  if (k != -1) {
    Menu* sub_sub_menu = Menu::s_menus.rbegin()->second;
    sub_sub_menu->_call(k);
    delete sub_sub_menu;
  }

  delete sub_menu;
}

EMSCRIPTEN_BINDINGS(menu) {
  emscripten::function("select", &select);
}

#endif

}

