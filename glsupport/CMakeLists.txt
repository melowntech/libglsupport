# bump version here
set(glsupport_VERSION 1.0)
define_module(LIBRARY glsupport=${glsupport_VERSION}
  DEPENDS math>=1.4 dbglog>=1.4 OpenGL::EGL OpenGL::GL
  DEFINITIONS MESA_EGL_NO_X11_HEADERS=1 GL_GLEXT_PROTOTYPES=1
  )

set(glsupport_SOURCES
  egl.hpp egl.cpp
  shader.hpp shader.cpp
  fb.hpp fb.cpp
  )

add_library(glsupport STATIC ${glsupport_SOURCES})

target_link_libraries(glsupport ${MODULE_LIBRARIES} ${CMAKE_DL_LIBS})
buildsys_target_compile_definitions(glsupport ${MODULE_DEFINITIONS})
buildsys_library(glsupport)
