/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_GRAPHICS_GBM_GBM_BUFFER_H_
#define MIR_GRAPHICS_GBM_GBM_BUFFER_H_

#include "mir/compositor/buffer.h"

#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdexcept>
#include <memory>

namespace mir
{
namespace graphics
{
namespace gbm
{

struct GBMBufferObjectDeleter
{
    void operator()(gbm_bo* handle) const;
};

geometry::PixelFormat gbm_format_to_mir_format(uint32_t format);
uint32_t mir_format_to_gbm_format(geometry::PixelFormat format);


class GBMBuffer: public compositor::Buffer
{
public:
    GBMBuffer(std::unique_ptr<gbm_bo, GBMBufferObjectDeleter> handle);
    GBMBuffer(const GBMBuffer&) = delete;
    virtual ~GBMBuffer();

    GBMBuffer& operator=(const GBMBuffer&) = delete;

    virtual geometry::Size size() const;

    virtual geometry::Stride stride() const;

    virtual geometry::PixelFormat pixel_format() const;

    virtual std::shared_ptr<compositor::BufferIPCPackage> get_ipc_package() const;

    virtual void bind_to_texture();

private:
    void ensure_egl_image();
    void ensure_egl_image_extensions();

    std::unique_ptr<gbm_bo, GBMBufferObjectDeleter> gbm_handle;
    EGLImageKHR egl_image;

    static PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
    static PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
    static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;
};

}
}
}


#endif // MIR_GRAPHICS_GBM_GBM_BUFFER_H_
