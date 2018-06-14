/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <epoxy/egl.h>

#include "display.h"
#include "egl_output.h"

#include "kms-utils/drm_mode_resources.h"

#include "mir/graphics/display_configuration.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/overlapping_output_grouping.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/virtual_output.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/transformation.h"
#include "mir/renderer/gl/render_target.h"
#include "mir/renderer/gl/context.h"

#include <xf86drmMode.h>
#include <sys/ioctl.h>
#include <system_error>
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mge = mir::graphics::eglstream;
namespace mgk = mir::graphics::kms;

namespace
{
EGLConfig choose_config(EGLDisplay display, mg::GLConfig const& requested_config)
{
    EGLint const config_attr[] = {
        EGL_SURFACE_TYPE, EGL_STREAM_BIT_KHR,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 0,
        EGL_DEPTH_SIZE, requested_config.depth_buffer_bits(),
        EGL_STENCIL_SIZE, requested_config.stencil_buffer_bits(),
        EGL_RENDERABLE_TYPE, MIR_SERVER_EGL_OPENGL_BIT,
        EGL_NONE
    };

    EGLint num_egl_configs;
    EGLConfig egl_config;
    if (eglChooseConfig(display, config_attr, &egl_config, 1, &num_egl_configs) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to chose EGL config"));
    }
    else if (num_egl_configs != 1)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{"Failed to find compatible EGL config"});
    }

    return egl_config;
}

EGLContext create_context(EGLDisplay display, EGLConfig config)
{
    eglBindAPI(MIR_SERVER_EGL_OPENGL_API);

    EGLint const context_attr[] = {
#if MIR_SERVER_EGL_OPENGL_BIT == EGL_OPENGL_ES2_BIT
        EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
        EGL_NONE
    };

    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attr);
    if (context == EGL_NO_CONTEXT)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
    }

    return context;
}

EGLContext create_context(EGLDisplay display, EGLConfig config, EGLContext shared_context)
{
    eglBindAPI(MIR_SERVER_EGL_OPENGL_API);

    EGLint const context_attr[] = {
#if MIR_SERVER_EGL_OPENGL_BIT == EGL_OPENGL_ES2_BIT
        EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
        EGL_NONE
    };

    EGLContext context = eglCreateContext(display, config, shared_context, context_attr);
    if (context == EGL_NO_CONTEXT)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
    }

    return context;
}

class DisplayBuffer
    : public mg::DisplaySyncGroup,
      public mg::DisplayBuffer,
      public mg::NativeDisplayBuffer,
      public mir::renderer::gl::RenderTarget
{
public:
    DisplayBuffer(EGLDisplay dpy, EGLContext ctx, EGLConfig config, mge::kms::EGLOutput const& output)
        : dpy{dpy},
          ctx{create_context(dpy, config, ctx)},
          layer{output.output_layer()},
          view_area_{output.extents()},
          transform{output.transformation()}
    {
        EGLint const stream_attribs[] = {
            EGL_STREAM_FIFO_LENGTH_KHR, 1,
            EGL_NONE
        };
        output_stream = eglCreateStreamKHR(dpy, stream_attribs);
        if (output_stream == EGL_NO_STREAM_KHR)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGLStream"));
        }

        EGLAttrib swap_interval;
        if (eglQueryOutputLayerAttribEXT(dpy, layer, EGL_SWAP_INTERVAL_EXT, &swap_interval) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to query swap interval"));
        }

        if (eglOutputLayerAttribEXT(dpy, layer, EGL_SWAP_INTERVAL_EXT, 1) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to set swap interval"));
        }

        if (eglStreamConsumerOutputEXT(dpy, output_stream, output.output_layer()) == EGL_FALSE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to attach EGLStream to output"));
        };

        EGLint const surface_attribs[] = {
            EGL_WIDTH, output.size().width.as_int(),
            EGL_HEIGHT, output.size().height.as_int(),
            EGL_NONE,
        };
        surface = eglCreateStreamProducerSurfaceKHR(dpy, config, output_stream, surface_attribs);
        if (surface == EGL_NO_SURFACE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create StreamProducerSurface"));
        }

    }

    /* gl::RenderTarget */
    void make_current() override
    {
        if (eglMakeCurrent(dpy, surface, surface, ctx) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to make context current"));
        }
    }

    void release_current() override
    {
        if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to release context"));
        }
    }

    void swap_buffers() override
    {
        if (eglSwapBuffers(dpy, surface) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("eglSwapBuffers failed"));
        }
    }

    mir::geometry::Rectangle view_area() const override
    {
        return view_area_;
    }

    bool overlay(const mir::graphics::RenderableList& /*renderlist*/) override
    {
        return false;
    }

    glm::mat2 transformation() const override
    {
        return transform;
    }

    mir::graphics::NativeDisplayBuffer* native_display_buffer() override
    {
        return this;
    }

    void for_each_display_buffer(const std::function<void(mir::graphics::DisplayBuffer&)>& f) override
    {
        f(*this);
    }

    void post() override
    {

    }

    void bind() override
    {
    }

    std::chrono::milliseconds recommended_sleep() const override
    {
        return std::chrono::milliseconds{0};
    }

private:
    EGLDisplay dpy;
    EGLContext ctx;
    EGLOutputLayerEXT layer;
    mir::geometry::Rectangle const view_area_;
    glm::mat2 const transform;
    EGLStreamKHR output_stream;
    EGLSurface surface;
};

mge::KMSDisplayConfiguration create_display_configuration(
    mir::Fd const& drm_node,
    EGLDisplay dpy,
    EGLContext context)
{
    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, context) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION((
            mg::egl_error("Failed to make EGL context current for display construction")));
    }
    return mge::KMSDisplayConfiguration{drm_node, dpy};
}
}

mge::Display::Display(
    mir::Fd drm_node,
    EGLDisplay display,
    std::shared_ptr<DisplayConfigurationPolicy> const& configuration_policy,
    GLConfig const& gl_conf)
    : drm_node{drm_node},
      display{display},
      config{choose_config(display, gl_conf)},
      context{create_context(display, config)},
      display_configuration{create_display_configuration(this->drm_node, display, context)}
{
    auto ret = drmSetClientCap(drm_node, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    if (ret != 0)
    {
        BOOST_THROW_EXCEPTION(std::system_error(-ret, std::system_category(), "Request for Universal Planes support failed"));
    }

    ret = drmSetClientCap(drm_node, DRM_CLIENT_CAP_ATOMIC, 1);
    if (ret != 0)
    {
        BOOST_THROW_EXCEPTION(std::system_error(-ret, std::system_category(), "Request for Atomic Modesetting support failed"));
    }

    configuration_policy->apply_to(display_configuration);

    configure(display_configuration);
}

void mge::Display::for_each_display_sync_group(const std::function<void(DisplaySyncGroup&)>& f)
{
    for (auto& group : active_sync_groups)
    {
        f(*group);
    }
}

std::unique_ptr<mg::DisplayConfiguration> mge::Display::configuration() const
{
    return display_configuration.clone();
}

void mge::Display::configure(DisplayConfiguration const& conf)
{
    auto kms_conf = dynamic_cast<KMSDisplayConfiguration const&>(conf);
    active_sync_groups.clear();
    kms_conf.for_each_output([this](kms::EGLOutput const& output)
         {
             if (output.used)
             {
                 const_cast<kms::EGLOutput&>(output).configure(output.current_mode_index);
                 active_sync_groups.emplace_back(std::make_unique<::DisplayBuffer>(display, context, config, output));
             }
         });
}

void mge::Display::register_configuration_change_handler(
    EventHandlerRegister& /*handlers*/,
    DisplayConfigurationChangeHandler const& /*conf_change_handler*/)
{
}

void mge::Display::register_pause_resume_handlers(
    EventHandlerRegister& /*handlers*/,
    DisplayPauseHandler const& /*pause_handler*/,
    DisplayResumeHandler const& /*resume_handler*/)
{
}

void mge::Display::pause()
{

}

void mge::Display::resume()
{

}

std::shared_ptr<mg::Cursor> mge::Display::create_hardware_cursor()
{
    // TODO: Find the cursor plane, and use it.
    return nullptr;
}

std::unique_ptr<mg::VirtualOutput> mge::Display::create_virtual_output(int /*width*/, int /*height*/)
{
    return nullptr;
}

mg::NativeDisplay* mge::Display::native_display()
{
    return this;
}

std::unique_ptr<mir::renderer::gl::Context> mge::Display::create_gl_context()
{
    class GLContext : public renderer::gl::Context
    {
    public:
        GLContext(EGLDisplay display, EGLContext context)
            : display{display},
              context{context}
        {
        }

        void make_current() const override
        {
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context);
        }

        void release_current() const override
        {
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        }

    private:
        EGLDisplay display;
        EGLContext context;
    };
    return std::make_unique<GLContext>(display, context);
}

bool mge::Display::apply_if_configuration_preserves_display_buffers(
    mg::DisplayConfiguration const& /*conf*/)
{
    return false;
}

mg::Frame mge::Display::last_frame_on(unsigned) const
{
    /*
     * TODO: Implement this later when we have the hardware + driver to test on.
     *       If no proper hardware counters are available, just call
     *       AtomicFrame.increment_now() in post() above.
     */
    return {};
}
