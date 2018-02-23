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

#ifndef egl_hpp_included_
#define egl_hpp_included_

#include <new>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "dbglog/dbglog.hpp"

#include "./eglfwd.hpp"

namespace glsupport { namespace egl {

namespace detail {
const char* error();

struct PlaceHolder { PlaceHolder() {}; };
} // namespace detail

struct Error : std::runtime_error {
    Error(const std::string &msg) : std::runtime_error(msg) {}
};

struct MissingExtension : Error {
    MissingExtension(const std::string &msg) : Error(msg) {}
};

struct Device {
    ::EGLDeviceEXT device;

    typedef std::vector<Device> list;

    Device() : device() {}
    Device(::EGLDeviceEXT device) : device(device) {}

    operator bool() const { return device; }
};

/** Query for all available devices on the platform.
 */
Device::list queryDevices();

class Display {
public:
    typedef std::shared_ptr<std::remove_pointer< ::EGLDisplay>::type> Ptr;

    Display(::EGLNativeDisplayType nativeDisplay = EGL_DEFAULT_DISPLAY);

    Display(const Device &device);

    Display(detail::PlaceHolder) {}

    ::EGLDisplay operator*() const { return dpy_.get(); }
    operator ::EGLDisplay() const { return dpy_.get(); }

private:
    Ptr dpy_;
};

inline ::EGLConfig asEglConfig(::EGLConfig config) {
    return config;
}

inline ::EGLConfig asEglConfig(const std::vector< ::EGLConfig> &configs) {
    return configs.front();
}

inline const ::EGLint* asEglAttributes(const ::EGLint *attributes) {
    return attributes;
}

inline const ::EGLint* asEglAttributes(std::nullptr_t) {
    return nullptr;
}

inline const ::EGLint*
asEglAttributes(const std::initializer_list< ::EGLint> &attributes)
{
    return &*attributes.begin();
}

std::vector< ::EGLConfig>
chooseConfigs(const Display &dpy, const ::EGLint *attributes, int limit = 1);

std::vector< ::EGLConfig>
inline chooseConfigs(const Display &dpy
                     , const std::initializer_list< ::EGLint> &attributes
                     , int limit = 1)
{
    return chooseConfigs(dpy, &*attributes.begin(), limit);
}

class Surface {
private:
    typedef std::shared_ptr<std::remove_pointer< ::EGLSurface>::type> Ptr;

public:
    Surface(const Display &dpy, ::EGLSurface surface);

    ::EGLSurface operator*() const { return surface_.get(); }
    operator ::EGLSurface() const { return surface_.get(); }

private:
    Ptr surface_;
};

namespace detail {

Surface pbuffer(const Display &display, ::EGLConfig config
                , const ::EGLint *attributes);

} // namespace detail

template <typename ConfigType, typename AttributesType>
Surface pbuffer(const Display &display, const ConfigType &config
                , const AttributesType &attributes)
{
    return detail::pbuffer(display, asEglConfig(config)
                           , asEglAttributes(attributes));
}

template <typename ConfigType>
Surface pbuffer(const Display &display, const ConfigType &config
                , const std::initializer_list< ::EGLint> &attributes)
{
    return detail::pbuffer(display, asEglConfig(config)
                           , asEglAttributes(attributes));
}

class Context {
private:
    typedef std::shared_ptr<std::remove_pointer< ::EGLContext>::type> Ptr;

public:
    Context() : dpy_(detail::PlaceHolder()) {}
    Context(const Context&) = default;

    Context(const Display &dpy, ::EGLContext context);

    void makeCurrent(const Surface &surface) const;
    void makeCurrent(const Surface &draw, const Surface &read) const;

    ::EGLContext operator*() const { return context_.get(); }
    operator ::EGLContext() const { return context_.get(); }

private:
    Display dpy_;
    Ptr context_;
};

namespace detail {

Context context(const Display &display, ::EGLConfig config
                , ::EGLContext share, const ::EGLint *attributes);

} // namespace detail

template <typename ConfigType, typename AttributesType = std::nullptr_t>
Context context(const Display &display, const ConfigType &config
                , const AttributesType &attributes = nullptr
                , ::EGLContext share = EGL_NO_CONTEXT)
{
    return detail::context(display, asEglConfig(config), share
                           , asEglAttributes(attributes));
}

template <typename ConfigType>
Context context(const Display &display, const ConfigType &config
                , const std::initializer_list< ::EGLint> &attributes
                , ::EGLContext share = EGL_NO_CONTEXT)
{
    return detail::context(display, asEglConfig(config), share
                           , asEglAttributes(attributes));
}

} } // namespace glsupport::egl

#endif // egl_hpp_included_
