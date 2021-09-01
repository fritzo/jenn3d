//
// Created by Shawn Zhong on 8/29/21.
//

#ifndef JENN_WASM_H
#define JENN_WASM_H

#include <GL/glut.h>

// unsupported: accumulate buffer
// used by options in the camera menu
#define glAccum(...) NULL
#define glClearAccum(...) NULL

// unsupported: space ball
#define glutSpaceballButtonFunc(...) NULL
#define glutSpaceballMotionFunc(...) NULL
#define glutSpaceballRotateFunc(...) NULL

// unsupported: game mode
#define glutGameModeString(...) NULL
#define glutEnterGameMode(...) NULL
#define glutLeaveGameMode(...) NULL

// unsupported: window-related
#define glutInitWindowPosition(...) NULL
#define glutSetWindowTitle(...) NULL

// unsupported: pixel transformation
// used in capture
#define glPixelTransferf(...) NULL

// unsupported: polygonMode
// for mode == GL_LINE, we need to manually render them
// using GL_LINE_STRIP or GL_LINE_LOOP or GL_LINES
#define glPolygonMode(...) NULL

// unsupported: GL_POLYGON and GL_QUAD_STRIP
// replace with GL_TRIANGLE_FAN and GL_TRIANGLE_STRIP respectively
#undef GL_QUAD_STRIP
#undef GL_POLYGON
#define GL_QUAD_STRIP GL_TRIANGLE_STRIP
#define GL_POLYGON GL_TRIANGLE_FAN

// unsupported: rendering text
#define glRasterPos2i(...) NULL
#define glutBitmapCharacter(...) NULL
#define glutBitmapWidth(...) NULL

// unsupported: font
#undef GLUT_BITMAP_8_BY_13
#define GLUT_BITMAP_8_BY_13 NULL
#undef GLUT_BITMAP_9_BY_15
#define GLUT_BITMAP_9_BY_15 NULL
#undef GLUT_BITMAP_TIMES_ROMAN_10
#define GLUT_BITMAP_TIMES_ROMAN_10 NULL
#undef GLUT_BITMAP_TIMES_ROMAN_24
#define GLUT_BITMAP_TIMES_ROMAN_24 NULL
#undef GLUT_BITMAP_HELVETICA_10
#define GLUT_BITMAP_HELVETICA_10 NULL
#undef GLUT_BITMAP_HELVETICA_12
#define GLUT_BITMAP_HELVETICA_12 NULL
#undef GLUT_BITMAP_HELVETICA_18
#define GLUT_BITMAP_HELVETICA_18 NULL

// unsupported: system functions
#define abort(...) NULL
#define exit(...) NULL

#endif // JENN_WASM_H
