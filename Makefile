#makefile for Jenn3d

####### switch this to whatever is appropriate for your platform ##############

#COMPILE_TYPE = mac
#COMPILE_TYPE = mac_debug
#Note: CYGWIN now is synonim for MinGW
#COMPILE_TYPE = cygwin
COMPILE_TYPE = linux
#COMPILE_TYPE = debug
#COMPILE_TYPE = profile
#COMPILE_TYPE = devel

#### if you have libpng installed, uncomment this:

#HAVE_PNG = false
HAVE_PNG = true

######## leave everything else the same #######################################

CC = g++
CXX = g++

#OPT = -O3 -funroll-loops -pipe
OPT = -O3 -ffast-math -fomit-frame-pointer -funroll-loops -pipe -s
#OPT = -O3 -m32 -march=prescott -malign-double -mfpmath=sse -ffast-math -fomit-frame-pointer -funroll-loops -fprefetch-loop-arrays -ftree-vectorize -fno-exceptions -fno-check-new -pipe
WARNINGS = -Wall -Wextra -Werror -Wno-unused

#opengl stuff
GL_LINUX = -lglut -lGLU -lGL
GL_MAC = -framework OpenGL -framework Glut
GL_CYGWIN = -lglut32 -lglu32 -lopengl32

#png stuff
ifdef HAVE_PNG
	PNG_LINUX = -lpng
	PNG_MAC = -L/sw/lib -lpng -lz
	PNG_CYGWIN = -L/usr/lib -lpng -lz
	USR_CAPT = -DCAPTURE=4
	DEV_CAPT = -DCAPTURE=24
else
	PNG_LINUX =
	PNG_MAC =
	PNG_CYGWIN =
	USER_CAPT =
	DEVEL_CAPT =
endif

#compiler flags
ifeq ($(COMPILE_TYPE), mac)
	CPPFLAGS = -I/sw/include -DDEBUG_LEVEL=0 -DMAC_HACKS $(USR_CAPT)
	CXXFLAGS = $(OPT) -I/sw/include
	LDFLAGS  = $(OPT) -L/sw/libs
	LIBS = $(GL_MAC) $(PNG_MAC)
endif
ifeq ($(COMPILE_TYPE), mac_debug)
	CPPFLAGS = -I/sw/include -DDEBUG_LEVEL=2 -DMAC_HACKS $(USR_CAPT)
	CXXFLAGS = -I/sw/include -ggdb
	LDFLAGS  = -L/sw/lib -rdynamic -ggdb
	LIBS = $(GL_MAC) $(PNG_MAC)
endif
ifeq ($(COMPILE_TYPE), cygwin)
	CPPFLAGS = -I/usr/include -DDEBUG_LEVEL=0 -DCYGWIN_HACKS $(USR_CAPT)
	CXXFLAGS = $(OPT) -I/usr/include
	LDFLAGS  = $(OPT) -L/usr/lib
	LIBS = $(GL_CYGWIN) $(PNG_CYGWIN)
endif
ifeq ($(COMPILE_TYPE), linux)
	CPPFLAGS = -DDEBUG_LEVEL=0 $(USR_CAPT)
	CXXFLAGS = $(OPT)
	LDFLAGS  = $(OPT)
	LIBS = $(GL_LINUX) $(PNG_LINUX)
endif
ifeq ($(COMPILE_TYPE), debug)
	CPPFLAGS = -DDEBUG_LEVEL=2 $(DEV_CAPT)
	CXXFLAGS = $(WARNINGS) -ggdb
	LDFLAGS  = -rdynamic -ggdb
	LIBS = $(GL_LINUX) $(PNG_LINUX)
endif
ifeq ($(COMPILE_TYPE), profile)
	CPPFLAGS = -DDEBUG_LEVEL=0 $(DEV_CAPT)
	CXXFLAGS = -O2 -pg -ftest-coverage -fprofile-arcs
	LDFLAGS  = -O2 -pg -ftest-coverage -fprofile-arcs
	LIBS = $(GL_LINUX) $(PNG_LINUX)
endif
ifeq ($(COMPILE_TYPE), devel)
	CPPFLAGS = -DDEBUG_LEVEL=0 $(DEV_CAPT)
	CXXFLAGS = $(OPT)
	LDFLAGS  = $(OPT)
	LIBS = $(GL_LINUX) $(PNG_LINUX)
endif

#default target
all: jenn

#modules
definitions.o: definitions.C definitions.h
linalg.o: linalg.C linalg.h definitions.h
todd_coxeter.o: todd_coxeter.C todd_coxeter.h linalg.h definitions.h
go_game.o: go_game.C go_game.h linalg.h todd_coxeter.h definitions.h
drawing.o: drawing.C drawing.h linalg.h go_game.h aligned_vect.h definitions.h
trail.o: trail.C trail.h linalg.h aligned_vect.h definitions.h
animation.o: animation.C animation.h linalg.h definitions.h
projection.o: projection.C projection.h animation.h drawing.h trail.h linalg.h definitions.h
polytopes.o: polytopes.C polytopes.h projection.h animation.h drawing.h definitions.h
menus.o: menus.C menus.h main.h polytopes.h projection.h animation.h drawing.h definitions.h
aligned_alloc.o: aligned_alloc.C aligned_alloc.h

#final product
MAIN_O = main.o linalg.o menus.o todd_coxeter.o go_game.o polytopes.o animation.o projection.o drawing.o trail.o aligned_alloc.o definitions.o
main.o: main.C main.h linalg.h menus.h go_game.h trail.h polytopes.h drawing.h animation.h projection.h definitions.h
jenn: $(MAIN_O)
	$(CC) $(CXXFLAGS) -o jenn $(MAIN_O) $(LIBS)

profile: jenn
	./jenn -c 5 2 2 3 2 3 -v 3 -e 0 1 2 -f 02 03 12 13
	gcov drawing.C
	gprof -l -b jenn > jenn.prof
	gvim jenn.prof &
test: test.C
	$(CC) -o test test.C $(LIBS)
	
clean:
	rm -f core *.o jenn temp.* *.prof *.gcov *.da *.bb *.bbg gmon.out jenn_capture.png jenn_export.stl
