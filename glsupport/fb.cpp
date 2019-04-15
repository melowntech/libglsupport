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

#include "dbglog/dbglog.hpp"

#include "./fb.hpp"
#include "./glerror.hpp"

namespace glsupport {

void checkGl(const char *name)
{
    auto err(::glGetError());
    if (err != GL_NO_ERROR) {
        LOG(warn3) <<  "OpenGL error in <" <<  name << ">";
    }

    switch (err) {
    case GL_NO_ERROR:
        return;
    case GL_INVALID_ENUM:
        throw std::runtime_error("gl_invalid_enum");
    case GL_INVALID_VALUE:
        throw std::runtime_error("gl_invalid_value");
    case GL_INVALID_OPERATION:
        throw std::runtime_error("gl_invalid_operation");
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        throw std::runtime_error("gl_invalid_framebuffer_operation");
    case GL_OUT_OF_MEMORY:
        throw std::runtime_error("gl_out_of_memory");
    default:
        throw std::runtime_error("gl_unknown_error");
    }
}

namespace {

void checkGlFramebuffer()
{
    switch (::glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
    case GL_FRAMEBUFFER_COMPLETE:
        return;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        throw std::runtime_error("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        throw std::runtime_error("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
    case GL_FRAMEBUFFER_UNSUPPORTED:
        throw std::runtime_error("GL_FRAMEBUFFER_UNSUPPORTED");
    default:
        throw std::runtime_error("Unknown error with opengl framebuffer");
    }
}

} // namespace

FrameBuffer::FrameBuffer(const math::Size2 &size, bool alpha)
    : size_(size), pixelType_(alpha ? PixelType::rgba8 : PixelType::rgb8)
    , fbId_(), depthTextureId_(), colorTextureId_()
{
    init();
}
FrameBuffer::FrameBuffer(const math::Size2 &size, PixelType pixelType)
    : size_(size), pixelType_(pixelType)
    , fbId_(), depthTextureId_(), colorTextureId_()
{
    init();
}

void FrameBuffer::init()
{
    checkGl("pre-framebuffer check");

    // depth buffer
    ::glActiveTexture(GL_TEXTURE0 + 5);
    ::glGenTextures(1, &depthTextureId_);
    ::glBindTexture(GL_TEXTURE_2D, depthTextureId_);

    ::glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32,
                   size_.width, size_.height,
                   0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    checkGl("update depth texture");

    // color buffer
    ::glActiveTexture(GL_TEXTURE0 + 7);
    ::glGenTextures(1, &colorTextureId_);
    ::glBindTexture(GL_TEXTURE_2D, colorTextureId_);

    switch (pixelType_) {
    case PixelType::rgb8:
        ::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
                       size_.width, size_.height,
                       0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        break;

    case PixelType::rgba8:
        ::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                       size_.width, size_.height,
                       0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        break;

    case PixelType::rgb32f:
        ::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F,
                       size_.width, size_.height,
                       0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        break;

    case PixelType::rgba32f:
        ::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F,
                       size_.width, size_.height,
                       0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        break;
    }

    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    checkGl("update color texture");

    ::glGenFramebuffers(1, &fbId_);
    ::glBindFramebuffer(GL_FRAMEBUFFER, fbId_);
    ::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT
                             , GL_TEXTURE_2D, depthTextureId_, 0);
    ::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0
                             , GL_TEXTURE_2D, colorTextureId_, 0);

    checkGlFramebuffer();
    checkGl("update frame buffer");
}

FrameBuffer::~FrameBuffer()
{
    ::glDeleteFramebuffers(1, &fbId_);
    ::glDeleteTextures(1, &depthTextureId_);
    ::glDeleteTextures(1, &colorTextureId_);
}

} // namespace glsupport
