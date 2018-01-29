# bump version here
set(glsupport_VERSION 1.0)

find_library(EGL_LIBRARY NAMES EGL)
find_library(GLESv2_LIBRARY NAMES GLESv2)

define_module(LIBRARY glsupport=${glsupport_VERSION}
  DEPENDS dbglog>=1.4
  DEFINITIONS MESA_EGL_NO_X11_HEADERS=1
  )

set(glsupport_SOURCES
  egl.hpp egl.cpp
  gles2.hpp gles2.cpp
  )

add_library(glsupport STATIC ${glsupport_SOURCES})

target_link_libraries(glsupport
  ${MODULE_LIBRARIES}
  ${EGL_LIBRARY}
  ${GLESv2_LIBRARY}
  )
buildsys_target_compile_definitions(glsupport ${MODULE_DEFINITIONS})
buildsys_library(glsupport)
