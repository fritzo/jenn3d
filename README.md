# Jenn3d #

Jenn is a toy for playing with various quotients of Cayley graphs of finite
Coxeter groups on four generators. Jenn builds the graphs using the
Todd-Coxeter algorithm, embeds them into the 3-sphere, and stereographically
projects them onto euclidean 3-space. (The models really live in the
hypersphere so they looked curved in our flat space.) Jenn has some basic
motion models governing the six degrees of freedom of rotation of the
hypersphere.

## Installing ##

Binaries are available at [jenn3d.org](http://jenn3d.org).

To compile yourself,
1. extract and unzip 
2. edit makefile: specify the target type
3. make
4. run jenn or jenn.exe

For example:

    $ tar -xzf jenn.2006_07_28.tgz
    $ cd jenn3d
    $ vim Makefile  # uncomment your compile type
    $ make
    $ ./jenn
    
You may also need to install the Glut and (optionally) the libpng libraries.
Users of Debian-based systems such as Ubuntu or Mac+Fink can get this with

    $ sudo apt-get install freeglut3-dev
    $ sudo apt-get install libpng12-dev  # or libpng3 on for Macs

On Windows+Cygwin you will need glut32.dll.

## Example Arguments ##

    # free polytopes
    -c 5 2 2 3 2 3 # (3,3,5)-polytope
    -c 3 2 2 4 2 3 # (3,4,3)-polytope
    -c 4 2 2 3 2 3 # (3,3,4)-polytope
    -c 3 2 2 3 2 3 # (3,3,3)-polytope
    
    # free polyhedra (2x)
    -c 12 2 2 2 2 12 # (12,12)-torus
    -c 3 2 2 5 2 2 # (3,5)-polyhedron
    -c 3 2 2 4 2 2 # (3,4)-polyhedron
    -c 3 2 2 3 2 2 # (3,3)-polyhedron
    
    # polytopes with complete circles
    -c 3 2 2 3 2 5   -v 1 2 3 # 600-cell
    -c 3 2 2 4 2 3   -v 1 2 3 # 24-cell
    -c 3 2 2 3 2 4   -v 1 2 3 # 16-cell
    -c 3 2 2 3 2 3   -v 1 2 # ???
    -c 12 2 2 2 2 12 -v 1 2 3 # a circle (boring)
    
    # polytopes with complete spheres
    # those above, but the 24-cell needs mods:
    -c 3 2 2 4 2 3 -v 1 2 3 -f 1210 01 -e 10 12101210 #24-cell
    
    # minimal spanning
    -c 5 2 2 3 2 3 -v 1 2 3 # 120-cell
    -c 4 2 2 3 2 3 -v 1 2 3 # 8-cell (hypercube)
    -c 4 2 2 2 2 4  # 8-cell (hypercube)
    -c 4 2 2 2 2 4  # 9-cell
    -c 3 2 2 3 2 3 -v 1 2 3 # 5-cell (simplex)
    -c 3 2 2 2 5 2 -v 2 3 # buckyball
    
    # misc
    -c 3 2 2 4 2 3 -v 0 2 3 # ???
    -c 3 2 2 3 2 3 -v 0 2 # ???
    
    # with faces
    -c 4 2 2 3 2 3 -v 1 2 3 -e 0 -f 01 # hypercube
    -c 5 2 2 3 2 3 -v 1 2 3 -e 0 -f 01 # 120-cell
    -c 3 2 2 3 2 5 -v 1 2 3 -e 0 -f 01 # 600-cell
    
    # solids
    -c 7 2 2 2 2 7 -e 0 1 2 3 -f 02 03 12 13 # a torus
    -c 4 2 2 3 2 3 -v 1 2 -e 0 -f 01 # lots of cubes
    -c 3 2 2 3 2 4 -v 2 3 -e 1 -f 12 # lots of octahedra
    -c 5 2 2 3 2 3 -v 1 2 -e 0 -f 01 -w 1 2 2 2 # lots of dodecahedra
    -c 3 2 2 3 2 5 -v 2 3 -e 1 -f 12 -w 2 3 3 3 # lots of icosahedra
    -c 5 2 2 3 2 3 -v 2 3 -e 1 -f 12 # lots of tetrahedra
    -c 3 2 2 4 2 3 -e 0 1 3 -f 01 03 13 # lots of hexagonal prisms
    -c 4 2 2 3 2 3 -e 0 1 3 -f 01 03 13 # lots of octagonal prisms
    
    # mazes
    -c 3 2 2 5 2 2 -e 0 1 2 3 -f 01 02 13 23 # prairie of glass
    -c 3 2 2 3 2 3 -e 0 1 2 3 -f 02 03 12 13 # house of glass
    -c 4 2 2 3 2 3 -e 0 1 2 3 -f 02 03 12 13 # block of glass
    -c 3 2 2 4 2 3 -e 0 1 2 3 -f 01 02 13 23 # town of glass
    -c 5 2 2 3 2 3 -v 0 2 -e 0 1 2 3 -f 12 13 # city of glass 0
    -c 5 2 2 3 2 3 -v 3 -e 0 1 2 -f 02 03 12 13 # city of glass 1
    -c 5 2 2 3 2 3 -v 2 -e 0 1 3 -f 02 03 12 13 # city of glass 2
    -c 5 2 2 3 2 3 -e 0 2 1 3 -f 02 03 12 13 -w 2 1 1 1 # world of glass
    
    # subgroups
    -c 3 2 2 4 2 3 -g 01 02 03 12 13 23 -e 01 02 03 12 13 23 -f 01 02 03 12 13 23

## License ##

Copyright (c) 2001-2010 Fritz Obermeyer
Licensed under the [GNU Public License version 2](http://www.gnu.org/copyleft/gpl.html)
