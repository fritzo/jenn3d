cmake_minimum_required(VERSION 3.11)
project(jenn3d)

set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
    add_compile_definitions(DEBUG_LEVEL=2)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -fomit-frame-pointer -funroll-loops -pipe")

add_executable(${PROJECT_NAME}
        main.C main.h
        definitions.C definitions.h
        linalg.C linalg.h
        todd_coxeter.C todd_coxeter.h
        go_game.C go_game.h
        drawing.C drawing.h
        trail.C trail.h
        animation.C animation.h
        projection.C projection.h
        menus.C menus.h
        polytopes.C polytopes.h
        aligned_alloc.C aligned_alloc.h
        aligned_vect.h
        glut_wasm.h
)

if(EMSCRIPTEN)
    message(STATUS "WebAssembly build enabled")

    set_target_properties(${PROJECT_NAME} 
        PROPERTIES SUFFIX ".js"
        LINK_FLAGS
            "--bind -s WASM=1 -s LEGACY_GL_EMULATION=1 -s ENVIRONMENT=web"
    )

    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD VERBATIM
        COMMAND ${CMAKE_COMMAND} -E
            copy ${PROJECT_NAME}.wasm ${PROJECT_NAME}.js
            ${CMAKE_CURRENT_SOURCE_DIR}/web
    )
else()
    message(STATUS "WebAssembly build not enabled")

    find_package(OpenGL REQUIRED)
    find_package(GLUT REQUIRED)
    find_package(PNG)

    target_link_libraries(${PROJECT_NAME} OpenGL::GL GLUT::GLUT)

    if(PNG_FOUND)
        target_link_libraries(${PROJECT_NAME} PNG::PNG)
        add_compile_definitions(CAPTURE=4)
    else()
        message(STATUS "libpng library not found")
    endif()
endif()

