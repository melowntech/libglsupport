#include "./gles2.hpp"

namespace glsupport { namespace gles2 {

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

} } // namespace glsupport::gles2
