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

#include "main.h"

#ifdef CYGWIN_HACKS
    #define GLUT_STATIC
#endif
#if defined(__APPLE__) && defined(__MACH__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <cmath>
#include <cstdlib>
#include <cstdio> //for sprintf
#include <vector>
#include <utility>
#include <set>

#include "definitions.h"
#include "linalg.h"
#include "menus.h"

#define MAX_TIME_STEP 0.5f

//keyboard & mouse numbers
#define ENTERKEY 13
#define ESCKEY 27
#define SCROLL_UP   3
#define SCROLL_DOWN 4

//#define DOUBLE_CLICK

const Logging::Logger logger("main", Logging::INFO);

//[ motion ]---------------------
//pause control
void drift ();
void end_pause ()
{
    if (not projector->paused) return;
    projector->paused = false;
    time_difference();
    glutIdleFunc(drift);
}
void beg_pause (bool redraw)
{
    if (projector->paused) return;
    glutIdleFunc(NULL);
    projector->paused = true;
    time_difference();
    if (redraw) projector->display(); //to draw a paused image
}
void toggle_pause () { if (projector->paused) end_pause(); else beg_pause(); }

//interaction
void display ();
void mouse (int button, int state, int X, int Y)
{
#ifdef __EMSCRIPTEN__
    //simulate right/middle button using ctrl/alt
    if (button == GLUT_LEFT_BUTTON) {
        if (glutGetModifiers() & GLUT_ACTIVE_CTRL) {
            button = GLUT_RIGHT_BUTTON;
        } else if (glutGetModifiers() & GLUT_ACTIVE_ALT) {
            button = GLUT_MIDDLE_BUTTON;
        }
    }
#endif
    logger.debug() << "button " << button << " is in state " << state |0;


#ifdef __EMSCRIPTEN__
    //scrolling controls zoom
    if ((button == SCROLL_UP || button == SCROLL_DOWN) && state == GLUT_DOWN) {
        projector->zoom(button == SCROLL_UP ? ZOOM_OUT : ZOOM_IN);
        return;
    }
#else
    //scrolling controls zoom
    if (button == SCROLL_UP) {
        if (state == GLUT_UP) projector->zoom(ZOOM_IN);
        update_title();
        return;
    } else if (button == SCROLL_DOWN) {
        if (state == GLUT_UP) projector->zoom(ZOOM_OUT);
        update_title();
        return;
    }

    //check for menu operation
    bool on_menu = Menus::Menu::mouse(button, state, X, Y);
    if (on_menu and state == GLUT_DOWN) return;
#endif

    //start dragging
    bool first_down = (animator->drag_channels == 0);
    if (first_down) {
        //begin dragging
        float x = projector->convert_x(X);
        float y = projector->convert_y(Y);
        animator->set_drag(x,y);

        end_pause();
        animator->reset_accel();
    }

    //update drag channels
    if (state == GLUT_DOWN) animator->drag_channels |=   1 << button;
    if (state == GLUT_UP)   animator->drag_channels &= ~(1 << button);

    //click on vertices
    //static float last_time = elapsed_time() - 1.0; //for double-clicking
    //static int last_button = 0;  //for double-clicking
    if (state == GLUT_DOWN) {
        int selected = projector->select(X, Y);
        if (selected < 0) return;

#ifdef DOUBLE_CLICK
        float time = elapsed_time();
        bool double_click = (button == last_button) and (time-last_time < 0.5);
        if (double_click) {
#endif
            switch (button) {
            case GLUT_LEFT_BUTTON:   drawing->play(selected, 1); break;
            case GLUT_RIGHT_BUTTON:  drawing->play(selected, 2); break;
            case GLUT_MIDDLE_BUTTON: drawing->play(selected, 0); break;
            }

#ifdef DOUBLE_CLICK
            last_button = 0;
            last_time = time - 1.0;
        } else {
            last_button = button;
            last_time = time;
        }
#endif
    }
}
void mouse_motion (int X, int Y)
{
    float x = projector->convert_x(X);
    float y = projector->convert_y(Y);
    animator->rot_drag(x, y);
}
void spaceball_button (int button, int state)
{
    enum {
        BUT_1 = 1, BUT_2, BUT_3, BUT_4, BUT_5, BUT_6, BUT_7, BUT_8,
        BUT_STAR, BUT_LEFT, BUT_RIGHT
    };
    switch (state) {
        case GLUT_DOWN:
            logger.info() << "pressed spaceball button " << button |0;
            break;
        case GLUT_UP:
            logger.info() << "released spaceball button " << button |0;
            break;
    }
}
void spaceball_motion (int X, int Y, int Z)
{
    const float scale = 1.0 / 1000.0;
    float x = scale * X;
    float y = scale * Y;
    float z = scale * Z;
    animator->set_trans_force(x,y,z);
}
void spaceball_rotate (int X, int Y, int Z)
{
    const float scale = 1.0 / 1800.0;
    float x = scale * X;
    float y = scale * Y;
    float z = scale * Z;
    animator->set_rot_force(x,y,z);
}
void drift ()
{
    float dt = min(time_difference(), MAX_TIME_STEP);
    if (dt) animator->drift(dt);
    glutPostRedisplay();
}
void keyboard (unsigned char key, int w_x, int w_y)
{
    switch (key) {
        case ESCKEY: exit(0);                           break;

        //XXX: these need to be updated
        case 'k': Menus::keyboard_help();               break;
        case 'h': //help
        case '?': //help
        case '/': Menus::main_help();                   break;
        case 'd': drawing->toggle_grid();               break;
        case 'v': drawing->toggle_verts();              break;
        case 'e': drawing->toggle_edges();              break;
        case 'f': drawing->toggle_faces();              break;
        case 'l': drawing->toggle_fancy();              break;
        case 'H': drawing->toggle_hazy();               break;
        case 'w': projector->toggle_wireframe();        break;
        case 'K': projector->toggle_contrast();         break;
        case 'r': projector->toggle_reversed();         break;
        case 'b': projector->toggle_blur();             break;
        case '1': projector->set_stereo(false);         break;
        case '2': projector->set_stereo(true);          break;
        case 'q': projector->toggle_quality();          break;
        case 'a': projector->scale_aperture(1/M_SQRT2); break;
        case 'A': projector->scale_aperture(M_SQRT2);   break;
        case 'n': projector->move_nearer();             break;
        case 'N': projector->move_farther();            break;
        case 'X': projector->reset();                   break;
        case 'm': animator->shift_channels();           break;
        case ' ': animator->toggle_stopped();           break;
        case 'c': animator->toggle_centered();          break;
        case 'C': animator->set_center();               break;
        case 'x': animator->reset();
                  projector->pan_center();              break;
        case 'P': projector->pan_center();              break;
        case 's': animator->slow_down();                break;
        case 'S': animator->speed_up();                 break;
        case 'D': animator->toggle_drifting();          break;
        case 'y': animator->toggle_flying();            break;
        case 't': projector->toggle_trail();            break;
        case 'T': projector->toggle_trailing();         break;
        case 'i': animator->invert();                   break;
        case 'p': toggle_pause();                       break;
        case 'F': toggle_fullscreen();                  break;
        case 'z': projector->zoom(ZOOM_IN); update_title(); break;
        case 'Z': projector->zoom(ZOOM_OUT); update_title(); break;
        case '=': //same as '+'
        case '+': drawing->set_tube_rad(drawing->get_tube_rad()*1.2f); break;
        case '-': drawing->set_tube_rad(drawing->get_tube_rad()/1.2f); break;
        case 'g': drawing->export_stl();                break;
        case 'G': drawing->export_graph();              break;
    }
}
void special_keys (int key, int, int)
{
    if (glutGetModifiers() & GLUT_ACTIVE_CTRL) {
        switch (key) {
            case GLUT_KEY_LEFT:  projector->pan_left (); break;
            case GLUT_KEY_RIGHT: projector->pan_right(); break;
            case GLUT_KEY_UP:    projector->pan_up   (); break;
            case GLUT_KEY_DOWN:  projector->pan_down (); break;
        }
        return;
    }

    if (glutGetModifiers() & GLUT_ACTIVE_SHIFT) {
        switch (key) {
            case GLUT_KEY_LEFT:  projector->pan_left (0.0625f); break;
            case GLUT_KEY_RIGHT: projector->pan_right(0.0625f); break;
            case GLUT_KEY_UP:    projector->pan_up   (0.0625f); break;
            case GLUT_KEY_DOWN:  projector->pan_down (0.0625f); break;
            //polytope selectors
            case GLUT_KEY_F1: Polytope::select(522323134); break;
            case GLUT_KEY_F2: Polytope::select(522323124); break;
            case GLUT_KEY_F3: Polytope::select(522323034); break;
            case GLUT_KEY_F4: Polytope::select(522323024); break;
            case GLUT_KEY_F5: Polytope::select(522323023); break;
            case GLUT_KEY_F6: Polytope::select(522323014); break;

            case GLUT_KEY_F7: Polytope::select(522323013); break;
            case GLUT_KEY_F8: Polytope::select(522323012); break;
            case GLUT_KEY_F9: Polytope::select(522323004); break;
            case GLUT_KEY_F10: Polytope::select(522323003); break;
            case GLUT_KEY_F11: Polytope::select(522323002); break;
            case GLUT_KEY_F12: Polytope::select(522323001); break;
        }
        update_title();
        return;
    }

    switch (key) {
        case GLUT_KEY_LEFT:  drawing->back();                       break;
        case GLUT_KEY_RIGHT: drawing->forward();                    break;
        case GLUT_KEY_DOWN:  projector->zoom(ZOOM_OUT);             break;
        case GLUT_KEY_UP:    projector->zoom(ZOOM_IN);              break;

        //polytope selectors
        case GLUT_KEY_F1: Polytope::select(Polytope::the_5_cell);   break;
        case GLUT_KEY_F2: Polytope::select(Polytope::the_8_cell);   break;
        case GLUT_KEY_F3: Polytope::select(Polytope::the_16_cell);  break;
        case GLUT_KEY_F4: Polytope::select(Polytope::the_24_cell);  break;
        case GLUT_KEY_F5: Polytope::select(Polytope::the_120_cell); break;
        case GLUT_KEY_F6: Polytope::select(Polytope::the_600_cell); break;

        case GLUT_KEY_F7: Polytope::select(Polytope::graph_torus);  break;

        case GLUT_KEY_F8: Polytope::select(Polytope::graph_333);    break;
        case GLUT_KEY_F9: Polytope::select(Polytope::graph_Y);      break;
        case GLUT_KEY_F10: Polytope::select(Polytope::graph_334);   break;
        case GLUT_KEY_F11: Polytope::select(Polytope::graph_343);   break;
        case GLUT_KEY_F12: Polytope::select(Polytope::graph_335);   break;
    }
    update_title();
} 

//displaying
void finish_buffer ()
{
    Menus::Menu::display();
    glutSwapBuffers();
}
void update_title ()
{
#ifndef __EMSCRIPTEN__
    static char title_string[256];
    sprintf(title_string, "Jenn. %.1f%%", 100.0 / animator->vis_rad);
    glutSetWindowTitle(title_string);
#endif
}
void display () { projector->display(); }
void reshape (int w, int h)
{
    Menus::Menu::reshape(w, h);
    projector->reshape(w, h);
}

//[ glut manager class ]----------
class GlutManager
{
    static int main_window;
public:
    GlutManager (int *argc, char **argv,
                 int init_width=800, int init_height=600,
                 bool show_start_msg=true);

    static void init_callbacks ();
    static void toggle_fullscreen ();
};
typedef GlutManager GM;
int GM::main_window;
GM *gl_manager = NULL;
GlutManager::GlutManager (int *argc, char **argv,
                          int init_width, int init_height,
                          bool show_start_msg)
{
    //define main window
    glutInit(argc,argv);

    //set display mode
#ifdef __EMSCRIPTEN__
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH
                       | GLUT_MULTISAMPLE );
#else
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH
                       | GLUT_ACCUM | GLUT_MULTISAMPLE );
    if (not glutGet(GLUT_DISPLAY_MODE_POSSIBLE)) {
        logger.warning()
            << "bad display mode: some features may be unavailable" |0;
        glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH );
    }
#endif

    //set window size
    glutInitWindowSize(init_width, init_height);
    projector->init_size(init_width, init_height);
    glutInitWindowPosition(50,50);
    main_window = glutCreateWindow(argv[0]);
    update_title();
    //glutGameModeString("1600x1200:16");
    glutGameModeString("");

    //startup
    init_callbacks();
    if (show_start_msg) Menus::start_menu();
    glutMainLoop();
}
void GlutManager::init_callbacks ()
{
    //set callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special_keys);
    glutMotionFunc(mouse_motion);
    glutSpaceballButtonFunc(spaceball_button);
    glutSpaceballMotionFunc(spaceball_motion);
    glutSpaceballRotateFunc(spaceball_rotate);
    glutIdleFunc(drift);

    //set parameters
    glClearColor(COLOR_BG,0.0);
    glClearAccum(0,0,0,1);
    glShadeModel(GL_FLAT);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
    //glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    //glEnable(GL_POLYGON_SMOOTH);
    //glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    //glHint(GL_FOG_HINT, GL_DONT_CARE);
#ifndef CYGWIN_HACKS
    glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
#endif
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glAccum(GL_MULT, 0.0f);
    //glDisable(GL_DITHER);

    drawing->update();
    Menus::RootMenu::open();
}
void GlutManager::toggle_fullscreen ()
{
#ifdef __EMSCRIPTEN__
  glutFullScreen();
#else
    //  this does not seem to work:
    //if (not glutGameModeGet(GLUT_GAME_MODE_POSSIBLE)) {
    //    logger.info() << "fullscreen not available" |0;
    //    return;
    //}
    if (not glutGameModeGet(GLUT_GAME_MODE_ACTIVE)) {
        glutEnterGameMode();
        init_callbacks();
        int w = glutGameModeGet(GLUT_GAME_MODE_WIDTH);
        int h = glutGameModeGet(GLUT_GAME_MODE_HEIGHT);
        reshape(w,h);
        //  this does not seem to work
        //int d = glutGameModeGet(GLUT_GAME_MODE_PIXEL_DEPTH);
        //logger.info() << "pixel depth = " << d |0;
    } else {
        glutLeaveGameMode();
        init_callbacks();
    }

    //this doesn't guarantee full-screen images.
    //glutFullScreen(); 
#endif
}
void toggle_fullscreen () { GM::toggle_fullscreen(); }

//[ main ]--------------------------------------------------
const char* const help_message =
"Usage: jenn [options]\n\
Starts Jenn3d [with specified model]\n\
Options:\n\
    -                           Do not print startup message\n\
    -c c12 c13 c14 c23 c24 c34  Give Coxeter matrix by upper triangle\n\
    -g [generators]             Subgroup generators, e.g. 2 3 4\n\
    -v [generators]             Vertex stabilizing generators, e.g. 1 2\n\
    -e edge [more edges]        Define an edge, e.g. 132322214\n\
    -f face [more faces]        Define a face, e.g. 34\n\
    -w w1 w2 w3 w4              Define the vertex weights, e.g. 3 2 2 1\n\
    -s width height             Set initial window size\n\
    -h, --help                  Display this message\n\
see notes.text for complete examples of command-line arguments\n";

int main(int argc,char **argv)
{
    Logging::title("Jenn. Copyright 2001-2007 Fritz Obermeyer.");

#ifndef CYGWIN_HACKS
    srand48(time_seed());
#endif

    //default polytope: 24-cell
    typedef Polytope::Word Word;
    Polytope::WordList gens, v_cogens, e_gens, f_gens;
    int coxeter[6] = {3,2,2,4,2,3};
    for (int i=0; i<4; ++i) {
        gens.push_back(Word(1,i));
    }
    v_cogens.push_back(Word(1,1));
    v_cogens.push_back(Word(1,2));
    v_cogens.push_back(Word(1,3));
    for (int i=0; i<4; ++i) {
        e_gens.push_back(Word(1,i));
        for (int j=i+1; j<4; ++j) {
                Word word(1,i);
                word.push_back(j);
                f_gens.push_back(word);
        }
    }
    Vect weights = const_vect(1);

    //default window settings
    int width = 800, height = 600;

    //read command-line options
    if (argc > 1) {
        v_cogens.clear(); //these are always cleared

        //skip command name
        int i=1;

        //parse generators & e_gens
        Polytope::WordList* words = &v_cogens;
        std::string _c("-c"), _g("-g"), _v("-v"), _e("-e"), _f("-f"), _w("-w");
        std::string _("-"), _s("-s"), _h("-h"), __help("--help");
        for (; i<argc; ++i) {
            const char* arg = argv[i];

            //ignore empty argument
            if (arg == _) continue;

            //set init window size
            if (arg == _s) {
                Assert (i+2 < argc, "too few size params (2 req'd)");
                width  = atoi(argv[i+1]);
                height = atoi(argv[i+2]);
                i += 2;
                continue;
            }

            //print help message
            if (arg == _h or arg == __help) {
                std::cout << help_message;
                return 0;
            }

            //read coxeter matrix
            if (arg == _c) {
                Assert (i+6 < argc, "too few Coxeter matrix entries (6 req'd)");
                for (int j=0; j<6; ++j) {
                    coxeter[j] = atoi(argv[i+j+1]);
                }
                i += 6;
                continue;
            }
            
            //start reading subgroup generators
            if (arg == _g) {
                gens.clear();
                e_gens.clear();
                f_gens.clear();
                words = &gens;
                continue;
            }
 
            //start reading vertex stabilizer subgroup generators
            if (arg == _v) {
                words = &v_cogens;
                continue;
            }

            //start reading edge generators
            if (arg == _e) {
                e_gens.clear();
                words = &e_gens;
                continue;
            }

            //start reading face generators
            if (arg == _f) {
                f_gens.clear();
                words = &f_gens;
                continue;
            }

            //read vector of weights (can occur in between other args)
            if (arg == _w) {
                Assert (i+4 < argc, "too few weights (4 required)");
                for (int j=0; j<4; ++j) {
                    weights[j] = atof(argv[i+1+j]);
                }
                i += 4;
                continue;
            }

            //otherwise read an element of a word
            words->push_back(Polytope::str2word(arg));
        }
    }

    //create drawing
    Polytope::view(coxeter, gens, v_cogens, e_gens, f_gens, weights);

    logger.debug() << "starting animator" |0;
    animator = new Animation::Animate();
    projector = new Projection::Projector();
    gl_manager = new GlutManager(&argc, argv, width, height, argc<=1);
    delete gl_manager;

    return 0;
}



