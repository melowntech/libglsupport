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

#ifndef shader_hpp_included_
#define shader_hpp_included_

#include <memory>

#include "utility/gl.hpp"

#include "dbglog/dbglog.hpp"

namespace glsupport {

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

} // namespace glsupport

#endif // shader_hpp_included_
