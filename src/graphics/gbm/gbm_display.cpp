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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/platform.h"
#include "mir/graphics/gbm/gbm_display.h"
#include "mir/graphics/gbm/gbm_display_helpers.h"
#include "mir/geometry/rectangle.h"

#include <stdexcept>
#include <xf86drm.h>

namespace mgg=mir::graphics::gbm;
namespace mggh=mir::graphics::gbm::helpers;
namespace mg=mir::graphics;
namespace geom=mir::geometry;

namespace
{

class BufferObject {
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

void bo_user_data_destroy(gbm_bo* /*bo*/, void *data)
{
    auto bufobj = reinterpret_cast<BufferObject*>(data);
    delete bufobj;
}

void page_flip_handler(int /*fd*/, unsigned int /*frame*/,
                       unsigned int /*sec*/, unsigned int /*usec*/,
                       void* data)
{
    bool* page_flip_pending{static_cast<bool*>(data)};
    *page_flip_pending = false;
}

}

class mgg::GBMDisplay::Private
{
public:
    Private()
        : last_flipped_bufobj{0}, page_flip_pending{false}
    {
    }

    ~Private()
    {
        if (last_flipped_bufobj)
            last_flipped_bufobj->release();
    }

    Private(const Private&) = delete;
    Private& operator=(const Private&) = delete;

    BufferObject* get_front_buffer_object();
    void wait_for_page_flip();

    BufferObject* last_flipped_bufobj;
    bool page_flip_pending;

    /* Order is important for construction/destruction */
    mggh::DRMHelper drm;
    mggh::KMSHelper kms;
    mggh::GBMHelper gbm;
    mggh::EGLHelper egl;
};

BufferObject* mgg::GBMDisplay::Private::get_front_buffer_object()
{
    auto bo = gbm_surface_lock_front_buffer(gbm.surface);
    if (!bo)
        return 0;

    /*
     * Check if we have already set up this gbm_bo (the gbm implementation is
     * free to reuse gbm_bos). If so, return the associated BufferObject.
     */
    auto bufobj = reinterpret_cast<BufferObject*>(gbm_bo_get_user_data(bo));
    if (bufobj)
        return bufobj;

    uint32_t handle = gbm_bo_get_handle(bo).u32;
    uint32_t stride = gbm_bo_get_stride(bo);
    uint32_t fb_id;

    /* Create a KMS FB object with the gbm_bo attached to it. */
    auto ret = drmModeAddFB(drm.fd, kms.mode.hdisplay, kms.mode.vdisplay,
                            24, 32, stride, handle, &fb_id);
    if (ret)
    {
        gbm_surface_release_buffer(gbm.surface, bo);
        return 0;
    }

    /* Create a BufferObject and associate it with the gbm_bo */ 
    bufobj = new BufferObject{gbm.surface, bo, fb_id};
    gbm_bo_set_user_data(bo, bufobj, bo_user_data_destroy);

    return bufobj;
}

void mgg::GBMDisplay::Private::wait_for_page_flip()
{
    /* Maximum time to wait for the page flip event in microseconds */
    static const long page_flip_max_wait_usec{100000};

    static drmEventContext evctx = {
        DRM_EVENT_CONTEXT_VERSION,  /* .version */
        0,  /* .vblank_handler */
        page_flip_handler  /* .page_flip_handler */
    };

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
        FD_SET(0, &fds);
        FD_SET(drm.fd, &fds);

        /* Wait for an event from the DRM device */
        auto ret = select(drm.fd + 1, &fds, NULL, NULL, &tv);

        if (ret > 0)
            drmHandleEvent(drm.fd, &evctx);
        else
            page_flip_pending = false;
    }
}

mgg::GBMDisplay::GBMDisplay()
    : priv{new Private{}}
{
    /* Set up all native resources */
    priv->drm.setup();
    priv->kms.setup(priv->drm);
    priv->gbm.setup(priv->drm, priv->kms.mode.hdisplay, priv->kms.mode.vdisplay);
    priv->egl.setup(priv->gbm);

    if (eglMakeCurrent(priv->egl.display, priv->egl.surface,
                       priv->egl.surface, priv->egl.context) == EGL_FALSE)
    {
        throw std::runtime_error("Failed to make EGL surface current");
    }

    if (eglSwapBuffers(priv->egl.display, priv->egl.surface) == EGL_FALSE)
        throw std::runtime_error("Failed to perform initial surface buffer swap");

    priv->last_flipped_bufobj = priv->get_front_buffer_object();
    auto ret = drmModeSetCrtc(priv->drm.fd, priv->kms.encoder->crtc_id,
                              priv->last_flipped_bufobj->get_drm_fb_id(), 0, 0,
                              &priv->kms.connector->connector_id, 1, &priv->kms.mode);
    if (ret)
        throw std::runtime_error("Failed to set DRM crtc");
}

mgg::GBMDisplay::~GBMDisplay() = default;

geom::Rectangle mgg::GBMDisplay::view_area() const
{
    geom::Rectangle rect;
    return rect;
}

bool mgg::GBMDisplay::post_update()
{
    /*
     * Bring the back buffer to the front and get the buffer object
     * corresponding to the front buffer.
     */
    if (eglSwapBuffers(priv->egl.display, priv->egl.surface) == EGL_FALSE)
        return false;

    auto bufobj = priv->get_front_buffer_object();
    if (!bufobj)
        return false;

    /*
     * Schedule the current front buffer object for display. Note that
     * drmModePageFlip is asynchronous and synchronized with vertical refresh,
     * so we tell DRM to emit a a page flip event with &page_flip_pending as
     * its user data when done.
     */
    auto ret = drmModePageFlip(priv->drm.fd, priv->kms.encoder->crtc_id,
                               bufobj->get_drm_fb_id(), DRM_MODE_PAGE_FLIP_EVENT,
                               &priv->page_flip_pending);
    if (ret)
    {
        /*
         * The buffer object won't be displayed. Release it to make
         * it available for future rendering.
         */
        bufobj->release();
        return false;
    }

    priv->page_flip_pending = true;

    /*
     * Wait for the current buffer object to be displayed/flipped, then release
     * the last flipped buffer object to make it available for future rendering.
     */
    priv->wait_for_page_flip();
    if (priv->last_flipped_bufobj)
        priv->last_flipped_bufobj->release();

    priv->last_flipped_bufobj = bufobj;

    return true;
}

std::shared_ptr<mg::Display> mg::create_display()
{
    return std::make_shared<mgg::GBMDisplay>();
}
