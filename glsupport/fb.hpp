#ifndef fb_hpp_included_
#define fb_hpp_included_

#include <memory>

#include <GL/gl.h>
#include <GL/glext.h>

#include "math/geometry_core.hpp"

namespace glsupport {

class FrameBuffer {
public:
    FrameBuffer(const math::Size2 &size, bool alpha = false);
    ~FrameBuffer();

private:
    const math::Size2 size_;
    const bool alpha_;

    ::GLuint fbId_;
    ::GLuint depthTextureId_;
    ::GLuint colorTextureId_;
};

} // namespace glsupport

#endif // fb_hpp_included_
