/**
 * Copyright (c) 2018 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <dlfcn.h>

#include <new>

#include "egl.hpp"

namespace glsupport { namespace egl {

namespace ext {

template <typename Prototype>
Prototype getProcAddress(const char *name, std::nothrow_t)
{
    ::dlerror();
    auto proc(Prototype(::dlsym(RTLD_DEFAULT, name)));
    if (auto error = ::dlerror()) {
        LOG(warn2)
            << "Unable to get address of EGL function: "
            << name << ": " << error << ".";
        return nullptr;
    }
    return Prototype(proc);
}

template <typename Prototype>
Prototype getProcAddress(const char *name)
{
    ::dlerror();
    auto proc(Prototype(::dlsym(RTLD_DEFAULT, name)));
    if (auto error = ::dlerror()) {
        LOGTHROW(err2, MissingExtension)
            << "Unable to get address of EGL function "
            << name << ": " << error << ".";
    }
    return Prototype(proc);
}

template <typename Prototype>
Prototype eglGetProcAddress(const char *name)
{
    typedef void* (EGLAPIENTRYP EglGetProcAddress)(const char *procname);
    static auto loader(getProcAddress<EglGetProcAddress>
                       ("eglGetProcAddress", std::nothrow));

    if (!loader) {
        LOGTHROW(err2, MissingExtension)
            << "EGL: unable to query extensions.";
    }

    auto proc(Prototype(loader(name)));
    if (!proc) {
        LOGTHROW(err2, MissingExtension)
            << "EGL: unable to get <" << name << "> extension.";
    }

    return proc;
}

::EGLDisplay getPlatformDisplay(const Device &device)
{
    static auto eglGetPlatformDisplayEXT
        (eglGetProcAddress<PFNEGLGETPLATFORMDISPLAYEXTPROC>
         ("eglGetPlatformDisplayEXT"));

    if (!eglGetPlatformDisplayEXT) {
        LOGTHROW(err2, MissingExtension)
            << "EGL: eglGetPlatformDisplayEXT unavailable.";
    }

    return eglGetPlatformDisplayEXT
        (EGL_PLATFORM_DEVICE_EXT, device.device, nullptr);
}

} // namespace ext

Device::list queryDevices()
{
    static auto eglQueryDevicesEXT
        (ext::eglGetProcAddress<PFNEGLQUERYDEVICESEXTPROC>
         ("eglQueryDevicesEXT"));

    if (!eglQueryDevicesEXT) {
        LOGTHROW(err2, MissingExtension)
            << "EGL: eglQueryDevicesEXT unavailable.";
    }

    ::EGLint deviceCount;
    if (!eglQueryDevicesEXT(0, nullptr, &deviceCount)) {
        LOGTHROW(err2, Error)
            << "EGL: Cannot query number of devices ("
            << detail::error() << ")";
    }

    std::vector< ::EGLDeviceEXT> devices;
    devices.resize(deviceCount);

    if (!eglQueryDevicesEXT(deviceCount, devices.data(), &deviceCount)) {
        LOGTHROW(err2, Error)
            << "EGL: Cannot query devices ("
            << detail::error() << ")";
    }

    // convert to device list
    Device::list out;
    for (auto device : devices) {
        out.emplace_back(device);
    }
    return out;
}

namespace {

template <typename What>
Display::Ptr openDisplay(::EGLDisplay dpy, What what)
{
    if (dpy == EGL_NO_DISPLAY) {
        LOGTHROW(err2, Error) << "EGL: No display found.";
    }

    EGLint major{};
    EGLint minor{};

    if (!::eglInitialize(dpy, &major, &minor)) {
        LOGTHROW(err2, Error)
            << "EGL: Cannot initialize display connection ("
            << detail::error() << ")";
    }

    Display::Ptr display(dpy, [](::EGLDisplay dpy) -> void
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

    LOG(info1) << "Initialized EGL display " << what
               << " (" << display << ", EGL version " << major
               << "." << minor << ").";
    return display;
}

} // namespace

Display::Display(::EGLNativeDisplayType nativeDisplay)
    : dpy_(openDisplay(::eglGetDisplay(nativeDisplay), nativeDisplay))
{}

Display::Display(const Device &device)
    : dpy_(openDisplay(ext::getPlatformDisplay(device), device.device))
{}

std::vector< ::EGLConfig> getConfigs(const Display &dpy, int limit)
{
    ::EGLint numConfigs;
    if (limit <= 0) {
        if (!::eglGetConfigs(dpy, nullptr, 0, &numConfigs)) {
            LOGTHROW(err2, Error)
                << "EGL: Cannot query number of available configurations ("
                << detail::error() << ").";
        }
        limit = numConfigs;
    }

    std::vector< ::EGLConfig> configs(limit);

    if (!::eglGetConfigs(dpy, configs.data(), limit, &numConfigs)) {
        LOGTHROW(err2, Error)
            << "EGL: Cannot get available configurations ("
            << detail::error() << ").";
    }
    configs.resize(numConfigs);

    return configs;
}

std::vector< ::EGLConfig>
chooseConfigs(const Display &dpy, const ::EGLint *attributes, int limit)
{
    ::EGLint numConfigs;
    if (limit <= 0) {
        if (!::eglChooseConfig(dpy, attributes, nullptr, 0, &numConfigs)) {
            LOGTHROW(err2, Error)
                << "EGL: Cannot query number of configurations ("
                << detail::error() << ").";
        }
        limit = numConfigs;
    }

    std::vector< ::EGLConfig> configs(limit);

    if (!::eglChooseConfig(dpy, attributes, configs.data()
                           , limit, &numConfigs))
    {
        LOGTHROW(err2, Error)
            << "EGL: Cannot choose configuration (" << detail::error() << ").";
    }
    configs.resize(numConfigs);

    return configs;
}

Surface::Surface(const Display &dpy, ::EGLSurface surface)
    : surface_(surface, [dpy](::EGLSurface surface) -> void
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

Surface pbuffer(const Display &dpy, ::EGLConfig config
                , const ::EGLint *attributes)
{
    auto surface(::eglCreatePbufferSurface(dpy, config, attributes));
    if (surface == EGL_NO_SURFACE) {
        LOGTHROW(err2, Error) << "EGL: Cannot create surface ("
                              << detail::error() << ").";
    }

    LOG(info1) << "EGL: Created surface " << surface
               << " at display " << dpy << ".";

    return Surface(dpy, surface);
}

} // namespace detail

Context::Context(const Display &dpy, ::EGLContext context)
    : dpy_(dpy)
    , context_(context, [dpy](::EGLContext context) -> void
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

Context context(const Display &dpy, ::EGLConfig config
                , ::EGLContext share, const ::EGLint *attributes)
{
    auto context(::eglCreateContext(dpy, config, share, attributes));
    if (context == EGL_NO_CONTEXT) {
        LOGTHROW(err2, Error)
            << "EGL: Cannot create context at display "
            << dpy << " (" << detail::error() << ").";
    }

    LOG(info1) << "EGL: Created context " << context
               << " at display " << dpy << ".";

    return Context(dpy, context);
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
