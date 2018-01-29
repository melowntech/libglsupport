#include "./egl.hpp"

namespace glsupport { namespace egl {

Display::Display(EGLNativeDisplayType nativeDisplay)
{
    auto dpy(::eglGetDisplay(nativeDisplay));

    if (dpy == EGL_NO_DISPLAY) {
        LOGTHROW(err2, Error)
            << "EGL: Display found (" << detail::error() << ").";
    }

    EGLint major{};
    EGLint minor{};

    if (!::eglInitialize(dpy, &major, &minor)) {
        LOGTHROW(err2, Error)
            << "EGL: Cannot initialize display connection ("
            << detail::error() << ")";
    }

    dpy_ = Ptr(dpy, [](EGLDisplay dpy) -> void
    {
        if (dpy == EGL_NO_DISPLAY) { return; }

        if (!::eglTerminate(dpy)) {
            LOG(err2)
                << "EGL: Unable to terminate connection to display "
                << dpy << " (" << detail::error() << ")";
            return;
        }

        LOG(info1) << "EGL: Closed connection to display "
                   << dpy << ".";
    });

    LOG(info1) << "Initialized EGL display " << nativeDisplay
               << " (" << dpy_ << ", EGL version " << major
               << "." << minor << ").";
}

std::vector<EGLConfig> getConfigs(const Display &dpy, int limit)
{
    EGLint numConfigs;
    if (limit <= 0) {
        if (!::eglGetConfigs(dpy, nullptr, 0, &numConfigs)) {
            LOGTHROW(err2, Error)
                << "EGL: Cannot query number of available configurations ("
                << detail::error() << ").";
        }
        limit = numConfigs;
    }

    std::vector<EGLConfig> configs(limit);

    if (!::eglGetConfigs(dpy, configs.data(), limit, &numConfigs)) {
        LOGTHROW(err2, Error)
            << "EGL: Cannot get available configurations ("
            << detail::error() << ").";
    }
    configs.resize(numConfigs);

    return configs;
}

std::vector<EGLConfig>
chooseConfigs(const Display &dpy, const EGLint *attributes, int limit)
{
    EGLint numConfigs;
    if (limit <= 0) {
        if (!::eglChooseConfig(dpy, attributes, nullptr, 0, &numConfigs)) {
            LOGTHROW(err2, Error)
                << "EGL: Cannot query number of configurations ("
                << detail::error() << ").";
        }
        limit = numConfigs;
    }

    std::vector<EGLConfig> configs(limit);

    if (!::eglChooseConfig(dpy, attributes, configs.data()
                           , limit, &numConfigs))
    {
        LOGTHROW(err2, Error)
            << "EGL: Cannot choose configuration (" << detail::error() << ").";
    }
    configs.resize(numConfigs);

    return configs;
}

Surface::Surface(const Display &dpy, EGLSurface surface)
    : surface_(surface, [dpy](EGLSurface surface) -> void
               {
                   if (surface == EGL_NO_SURFACE) { return; }

                   if (!::eglDestroySurface(dpy, surface)) {
                       LOG(err2)
                           << "EGL: Unable to destroy surface "
                           << surface << ".";
                       return;
                   }

                   LOG(info1) << "EGL: Destroyed surface " << surface
                              << ".";
               })
{}

namespace detail {

Surface pbuffer(const Display &display, EGLConfig config
                , const EGLint *attributes)
{
    auto surface(::eglCreatePbufferSurface(display, config, attributes));
    if (surface == EGL_NO_SURFACE) {
        LOGTHROW(err2, Error) << "EGL: Cannot create surface ("
                              << detail::error() << ").";
    }

    LOG(info1) << "EGL: Created surface " << surface
               << " at display " << display << ".";

    return { display, surface };
}

} // namespace detail

Context::Context(const Display &dpy, EGLContext context)
    : dpy_(dpy)
    , context_(context, [dpy](EGLContext context) -> void
               {
                   if (context == EGL_NO_CONTEXT) { return; }

                   if (!::eglDestroyContext(dpy, context)) {
                       LOG(err2)
                           << "EGL: Unable to destroy context "
                           << context << ".";
                       return;
                   }

                   LOG(info1) << "EGL: Destroyed context " << context
                              << ".";
               })
{}

void Context::makeCurrent(const Surface &surface) const
{
    if (!::eglMakeCurrent(dpy_, surface, surface, context_.get())) {
        LOGTHROW(err1, Error)
            << "EGL: Cannot make context " << context_
            << " current on display " << dpy_
            << " (" << detail::error() << ").";
    }
}

void Context::makeCurrent(const Surface &draw, const Surface &read) const
{
    if (!::eglMakeCurrent(dpy_, draw, read, context_.get())) {
        LOGTHROW(err1, Error)
            << "EGL: Cannot make context " << context_
            << " current on display " << dpy_
            << " (" << detail::error() << ").";
    }
}


namespace detail {

Context context(const Display &display, EGLConfig config
                , EGLContext share, const EGLint *attributes)
{
    auto context(::eglCreateContext(display, config, share, attributes));
    if (context == EGL_NO_CONTEXT) {
        LOGTHROW(err2, Error)
            << "EGL: Cannot create context at display "
            << display << " (" << detail::error() << ").";
    }

    LOG(info1) << "EGL: Created context " << context
               << " at display " << display << ".";

    return { display, context };
}

const char* error()
{
    switch (eglGetError()) {
    case EGL_SUCCESS:
        return "The last function succeeded without error.";
    case EGL_NOT_INITIALIZED:
        return "EGL is not initialized, or could not be initialized, for "
            "the specified EGL display connection.";
    case EGL_BAD_ACCESS:
        return "EGL cannot access a requested resource";
    case EGL_BAD_ALLOC:
        return "EGL failed to allocate resources for the requested operation.";
    case EGL_BAD_ATTRIBUTE:
        return "An unrecognized attribute or attribute value was passed "
            "in the attribute list.";
    case EGL_BAD_CONFIG:
        return "An EGLConfig argument does not name a valid EGL frame buffer "
            "configuration.";
    case EGL_BAD_CONTEXT:
        return "An EGLContext argument does not name a valid EGL "
            "rendering context.";
    case EGL_BAD_CURRENT_SURFACE:
        return "The current surface of the calling thread is a window, "
            "pixel buffer or pixmap that is no longer valid.";
    case EGL_BAD_DISPLAY:
        return "An EGLDisplay argument does not name a valid EGL "
            "display connection.";
    case EGL_BAD_MATCH:
        return "Arguments are inconsistent (for example, a valid context "
            "requires buffers not supplied by a valid surface).";
    case EGL_BAD_NATIVE_PIXMAP:
        return "A NativePixmapType argument does not refer to a "
            "valid native pixmap.";
    case EGL_BAD_NATIVE_WINDOW:
        return "A NativeWindowType argument does not refer to a "
            "valid native window.";
    case EGL_BAD_PARAMETER:
        return "One or more argument values are invalid.";
    case EGL_BAD_SURFACE:
        return "An EGLSurface argument does not name a valid surface (window, "
            "pixel buffer or pixmap) configured for GL rendering.";
    case EGL_CONTEXT_LOST:
        return "A power management event has occurred. The application must "
            "destroy all contexts and reinitialise OpenGL ES state and "
            "objects to continue rendering.";
    }

    return "Unknown error.";
}

} // namespace detail

} } // namespace glsupport::egl
