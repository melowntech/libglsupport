#ifndef egl_hpp_included_
#define egl_hpp_included_

#include <EGL/egl.h>

#include "dbglog/dbglog.hpp"

namespace glsupport { namespace egl {

namespace detail {
const char* error();

struct PlaceHolder { PlaceHolder() {}; };
} // namespace detail

struct Error : std::runtime_error {
    Error(const std::string &msg) : std::runtime_error(msg) {}
};

class Display {
private:
    typedef std::shared_ptr<std::remove_pointer< ::EGLDisplay>::type> Ptr;

public:
    Display(::EGLNativeDisplayType nativeDisplay = EGL_DEFAULT_DISPLAY);

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
