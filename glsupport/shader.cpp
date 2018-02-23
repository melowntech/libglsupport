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

#include "./shader.hpp"

namespace glsupport {

namespace detail {

std::shared_ptr< ::GLuint> loadShader(::GLenum type, const void *data
                                      , std::size_t size)
{
    std::shared_ptr< ::GLuint> shader
        (new ::GLuint(), [](GLuint *shader) -> void {
            ::glDeleteShader(*shader);
        });

    *shader = ::glCreateShader(type);

    if (!*shader) {
        LOGTHROW(err2, Error)
            << "Cannot create GL shader.";
    }

    const ::GLchar *d(static_cast<const GLchar*>(data));
    const ::GLint l(size);
    ::glShaderSource(*shader, GLsizei(1), &d, &l);

    ::glCompileShader(*shader);

    ::GLint compiled{};
    ::glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLint il = 0;
        ::glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &il);
        if (il > 1) {
            std::vector<char> log(il);

            ::glGetShaderInfoLog(*shader, il, nullptr, &log[0]);
            LOGTHROW(err2, Error)
                << "Cannot compile shader: " << log.data();
        }

        LOGTHROW(err2, Error)
            << "Cannot compile shader.";
    }

    return shader;
}

} // namespace detail

void Program::link(VertexShader vs, FragmentShader fs
                   , const Attributes &attributes)
{
    // create invalid program
    Ptr program(new ::GLuint(0), [](::GLuint *program)
                {
                    ::glDeleteProgram(*program);
                });

    // having valid pointer we can safely create program
    *program = ::glCreateProgram();
    if (!*program) {
        LOGTHROW(err2, Error)
            << "Cannot create shader.";
    }

    ::glAttachShader(*program, vs);
    ::glAttachShader(*program, fs);

    for (const auto &attr : attributes.attrs) {
        ::glBindAttribLocation(*program, attr.first, attr.second);
    }

    ::glLinkProgram(*program);

    ::GLint linked{};
    ::glGetProgramiv(*program, GL_LINK_STATUS, &linked);

    if (!linked) {
        ::GLint il = 0;
        ::glGetProgramiv(*program, GL_INFO_LOG_LENGTH, &il);
        if (il > 1) {
            std::vector<char> log;
            log.resize(il);

            ::glGetProgramInfoLog(*program, il, nullptr, log.data());
            LOGTHROW(err2, Error)
                << "Cannot link program: " << log.data();
        }

        LOGTHROW(err2, Error)
            << "Cannot link program.";
    }

    program_ = std::move(program);
    vs_ = std::move(vs);
    fs_ = std::move(fs);
}

} // namespace glsupport
