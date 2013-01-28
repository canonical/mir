/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "gbm_display_buffer.h"
#include "gbm_platform.h"
#include "mir/graphics/display_listener.h"

#include <boost/throw_exception.hpp>
#include <GLES2/gl2.h>

#include <stdexcept>

namespace mgg = mir::graphics::gbm;
namespace geom = mir::geometry;

class mgg::BufferObject
{
public:
    BufferObject(gbm_surface* surface, gbm_bo* bo, uint32_t drm_fb_id)
        : surface{surface}, bo{bo}, drm_fb_id{drm_fb_id}
    {
    }

    ~BufferObject()
    {
        if (drm_fb_id)
        {
            int drm_fd = gbm_device_get_fd(gbm_bo_get_device(bo));
            drmModeRmFB(drm_fd, drm_fb_id);
        }
    }

    void release() const
    {
        gbm_surface_release_buffer(surface, bo);
    }

    uint32_t get_drm_fb_id() const
    {
        return drm_fb_id;
    }

private:
    gbm_surface *surface;
    gbm_bo *bo;
    uint32_t drm_fb_id;
};

namespace
{

void bo_user_data_destroy(gbm_bo* /*bo*/, void *data)
{
    auto bufobj = static_cast<mgg::BufferObject*>(data);
    delete bufobj;
}

void page_flip_handler(int /*fd*/, unsigned int /*frame*/,
                       unsigned int /*sec*/, unsigned int /*usec*/,
                       void* data)
{
    auto page_flip_pending = static_cast<bool*>(data);
    *page_flip_pending = false;
}

void ensure_egl_image_extensions()
{
    std::string ext_string;
    const char* exts = eglQueryString(eglGetCurrentDisplay(), EGL_EXTENSIONS);
    if (exts)
        ext_string = exts;

    if (ext_string.find("EGL_MESA_drm_image") == std::string::npos)
        BOOST_THROW_EXCEPTION(std::runtime_error("EGL implementation doesn't support EGL_MESA_drm_image extension"));

    exts = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (exts)
        ext_string = exts;
    else
        ext_string.clear();

    if (ext_string.find("GL_OES_EGL_image") == std::string::npos)
        BOOST_THROW_EXCEPTION(std::runtime_error("GLES2 implementation doesn't support GL_OES_EGL_image extension"));
}

}

mgg::GBMDisplayBuffer::GBMDisplayBuffer(std::shared_ptr<GBMPlatform> const& platform,
                                        std::shared_ptr<DisplayListener> const& listener)
    : last_flipped_bufobj{nullptr},
      platform(platform),
      listener(listener),
      drm(platform->drm)
{
    /* Set up all native resources */
    kms.setup(drm);
    surface_gbm = platform->gbm.create_scanout_surface(kms.mode.hdisplay, kms.mode.vdisplay);
    egl.setup(platform->gbm, surface_gbm.get());

    listener->report_successful_setup_of_native_resources();

    make_current();

    listener->report_successful_egl_make_current_on_construction();

    ensure_egl_image_extensions();

    clear();

    if (eglSwapBuffers(egl.display, egl.surface) == EGL_FALSE)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to perform initial surface buffer swap"));

    listener->report_successful_egl_buffer_swap_on_construction();

    last_flipped_bufobj = get_front_buffer_object();
    auto ret = drmModeSetCrtc(drm.fd, kms.encoder->crtc_id,
                              last_flipped_bufobj->get_drm_fb_id(), 0, 0,
                              &kms.connector->connector_id, 1, &kms.mode);
    if (ret)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to set DRM crtc"));

    listener->report_successful_drm_mode_set_crtc_on_construction();
    listener->report_successful_display_construction();
}

mgg::GBMDisplayBuffer::~GBMDisplayBuffer()
{
    /*
     * There is no need to destroy last_flipped_bufobj manually.
     * It will be destroyed when its gbm_surface gets destroyed.
     */
    if (last_flipped_bufobj)
        last_flipped_bufobj->release();
}

geom::Rectangle mgg::GBMDisplayBuffer::view_area() const
{
    return {{geom::X(0), geom::Y(0)},
            {geom::Width(kms.mode.hdisplay), geom::Height(kms.mode.vdisplay)}};
}

void mgg::GBMDisplayBuffer::clear()
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

bool mgg::GBMDisplayBuffer::post_update()
{
    /*
     * Bring the back buffer to the front and get the buffer object
     * corresponding to the front buffer.
     */
    if (eglSwapBuffers(egl.display, egl.surface) == EGL_FALSE)
        return false;

    auto bufobj = get_front_buffer_object();
    if (!bufobj)
        return false;

    /*
     * Schedule the current front buffer object for display, and wait
     * for it to be actually displayed (flipped).
     *
     * If the flip fails, release the buffer object to make it available
     * for future rendering.
     */
    if (!schedule_and_wait_for_page_flip(bufobj))
    {
        bufobj->release();
        return false;
    }

    /*
     * Release the last flipped buffer object (which is not displayed anymore)
     * to make it available for future rendering.
     */
    if (last_flipped_bufobj)
        last_flipped_bufobj->release();

    last_flipped_bufobj = bufobj;

    return true;
}

mgg::BufferObject* mgg::GBMDisplayBuffer::get_front_buffer_object()
{
    auto bo = gbm_surface_lock_front_buffer(surface_gbm.get());
    if (!bo)
        return nullptr;

    /*
     * Check if we have already set up this gbm_bo (the gbm implementation is
     * free to reuse gbm_bos). If so, return the associated BufferObject.
     */
    auto bufobj = static_cast<BufferObject*>(gbm_bo_get_user_data(bo));
    if (bufobj)
        return bufobj;

    uint32_t fb_id{0};
    auto handle = gbm_bo_get_handle(bo).u32;
    auto stride = gbm_bo_get_stride(bo);

    /* Create a KMS FB object with the gbm_bo attached to it. */
    auto ret = drmModeAddFB(drm.fd, kms.mode.hdisplay, kms.mode.vdisplay,
                            24, 32, stride, handle, &fb_id);
    if (ret)
    {
        gbm_surface_release_buffer(surface_gbm.get(), bo);
        return nullptr;
    }

    /* Create a BufferObject and associate it with the gbm_bo */
    bufobj = new BufferObject{surface_gbm.get(), bo, fb_id};
    gbm_bo_set_user_data(bo, bufobj, bo_user_data_destroy);

    return bufobj;
}


bool mgg::GBMDisplayBuffer::schedule_and_wait_for_page_flip(BufferObject* bufobj)
{
    /* Maximum time to wait for the page flip event in microseconds */
    static const long page_flip_max_wait_usec{100000};
    static drmEventContext evctx =
    {
        DRM_EVENT_CONTEXT_VERSION,  /* .version */
        0,  /* .vblank_handler */
        page_flip_handler  /* .page_flip_handler */
    };
    bool page_flip_pending{true};

    /*
     * Schedule the current front buffer object for display. Note that
     * drmModePageFlip is asynchronous and synchronized with vertical refresh,
     * so we tell DRM to emit a a page flip event with &page_flip_pending as
     * its user data when done.
     */
    auto ret = drmModePageFlip(drm.fd, kms.encoder->crtc_id,
                               bufobj->get_drm_fb_id(), DRM_MODE_PAGE_FLIP_EVENT,
                               &page_flip_pending);
    if (ret)
        return false;

    /*
     * Wait $page_flip_max_wait_usec for the page flip event. If we get the
     * page flip event, page_flip_handler(), called through drmHandleEvent(),
     * will reset the page_flip_pending flag. If we don't get the page flip
     * event within that time, or we can't read from the DRM fd, act as if the
     * page flip has occured anyway.
     *
     * The rationale is that if we don't get a page flip event "soon" after
     * scheduling a page flip, something is severely broken at the driver
     * level. In that case, acting as if the page flip has occured will not
     * cause any worse harm anyway (perhaps some tearing), and will allow us to
     * continue processing instead of just hanging.
     */
    while (page_flip_pending)
    {
        struct timeval tv{0, page_flip_max_wait_usec};
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(drm.fd, &fds);

        /* Wait for an event from the DRM device */
        auto ret = select(drm.fd + 1, &fds, NULL, NULL, &tv);

        if (ret > 0)
            drmHandleEvent(drm.fd, &evctx);
        else
            page_flip_pending = false;
    }

    return true;
}

void mgg::GBMDisplayBuffer::make_current()
{
    if (eglMakeCurrent(egl.display, egl.surface,
                       egl.surface, egl.context) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to make EGL surface current"));
    }
}
