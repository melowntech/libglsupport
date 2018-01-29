#ifndef gles2_hpp_included_
#define gles2_hpp_included_

#include <memory>

#include <GLES2/gl2.h>

#include "dbglog/dbglog.hpp"

namespace glsupport { namespace gles2 {

struct Error : std::runtime_error {
    Error(const std::string &msg) : std::runtime_error(msg) {}
};

namespace detail {
std::shared_ptr< ::GLuint> loadShader(::GLenum type, const void *data
                                      , std::size_t size);
} // namespace detail

template < ::GLenum Type>
class Shader {
public:
    static constexpr GLenum type = Type;

    Shader() {}

    template <typename T, int size>
    Shader(const T(&data)[size]) {
        load(data, size * sizeof(T));
    }

    Shader(const std::string &src) { load(src); }

    Shader(const char *src) { load(src, strlen(src));  }

    void load(const std::string &src) { load(src.data(), src.length()); }

    template <typename T, int size>
    void load(const T(&data)[size]) {
        load(data, size * sizeof(T));
    }

    GLuint get() const { return shader_ ? *shader_ : 0; }
    operator GLuint() const { return get(); }

private:
    void load(const void *data, std::size_t size) {
        shader_ = detail::loadShader(type, data, size);
    }

    typedef std::shared_ptr< ::GLuint> Ptr;
    Ptr shader_;
};

typedef Shader<GL_VERTEX_SHADER> VertexShader;
typedef Shader<GL_FRAGMENT_SHADER> FragmentShader;

class Program {
public:
    struct Attributes;

    Program() {}

    void link(VertexShader vs, FragmentShader fs);

    void link(VertexShader vs, FragmentShader fs
              , const Attributes &attributes);

    ::GLuint get() const { return program_ ? *program_ : 0; }
    operator ::GLuint() const { return get(); }

    void use() const { ::glUseProgram(get()); }

    void stop() const { ::glUseProgram(0); }

    ::GLint uniform(const char *name) const {
        return ::glGetUniformLocation(get(), name);
    }

    ::GLint uniform(const std::string &name) const {
        return uniform(name.c_str());
    }

    ::GLint attribute(const char *name) const {
        return ::glGetAttribLocation(get(), name);
    }

    ::GLint attribute(const std::string &name) const {
        return attribute(name.c_str());
    }

private:
    VertexShader vs_;
    FragmentShader fs_;

    typedef std::shared_ptr< ::GLuint> Ptr;
    Ptr program_;
};

struct Program::Attributes {
    typedef std::pair< ::GLuint, const ::GLchar *> Attr;
    typedef std::vector<Attr> AttrList;
    AttrList attrs;

    Attributes() {}

    Attributes(::GLuint index, const ::GLchar *name) {
        attrs.emplace_back(index, name);
    }

    Attributes& operator()(::GLuint index, const ::GLchar *name) {
        attrs.emplace_back(index, name);
        return *this;
    }
};

// inlines

inline void Program::link(VertexShader vs, FragmentShader fs)
{
    return link(vs, fs, {});
}

} } // namespace glsupport::gles2

#endif // gles2_hpp_included_
