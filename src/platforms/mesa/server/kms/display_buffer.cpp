/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "display_buffer.h"
#include "kms_output.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/transformation.h"
#include "bypass.h"
#include "gbm_buffer.h"
#include "mir/fatal.h"
#include "mir/log.h"
#include "native_buffer.h"
#include "mir/graphics/egl_error.h"

#include <boost/throw_exception.hpp>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include MIR_SERVER_GL_H
#include <GLES2/gl2ext.h>
#include <drm_fourcc.h>

#include <sstream>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <algorithm>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace geom = mir::geometry;

mgm::GBMOutputSurface::FrontBuffer::FrontBuffer()
    : surf{nullptr},
      bo{nullptr}
{
}

mgm::GBMOutputSurface::FrontBuffer::FrontBuffer(gbm_surface* surface)
    : surf{surface},
      bo{gbm_surface_lock_front_buffer(surface)}
{
    if (!bo)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to acquire front buffer of gbm_surface"));
    }
}

mgm::GBMOutputSurface::FrontBuffer::~FrontBuffer()
{
    if (surf)
    {
        gbm_surface_release_buffer(surf, bo);
    }
}

mgm::GBMOutputSurface::FrontBuffer::FrontBuffer(FrontBuffer&& from)
    : surf{from.surf},
      bo{from.bo}
{
    const_cast<gbm_surface*&>(from.surf) = nullptr;
    const_cast<gbm_bo*&>(from.bo) = nullptr;
}

auto mgm::GBMOutputSurface::FrontBuffer::operator=(FrontBuffer&& from) -> FrontBuffer&
{
    if (surf)
    {
        gbm_surface_release_buffer(surf, bo);
    }

    const_cast<gbm_surface*&>(surf) = from.surf;
    const_cast<gbm_bo*&>(bo) = from.bo;

    const_cast<gbm_surface*&>(from.surf) = nullptr;
    const_cast<gbm_bo*&>(from.bo) = nullptr;

    return *this;
}

auto mgm::GBMOutputSurface::FrontBuffer::operator=(std::nullptr_t) -> FrontBuffer&
{
    return *this = FrontBuffer{};
}

mgm::GBMOutputSurface::FrontBuffer::operator gbm_bo*()
{
    return bo;
}

mgm::GBMOutputSurface::FrontBuffer::operator bool() const
{
    return (surf != nullptr) && (bo != nullptr);
}

namespace
{
void require_extensions(
    std::initializer_list<char const*> extensions,
    std::function<std::string()> const& extension_getter)
{
    std::stringstream missing_extensions;

    std::string const ext_string = extension_getter();

    for (auto extension : extensions)
    {
        if (ext_string.find(extension) == std::string::npos)
        {
            missing_extensions << "Missing " << extension << std::endl;
        }
    }

    if (!missing_extensions.str().empty())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            std::string("Missing required extensions:\n") + missing_extensions.str()));
    }
}

void require_egl_extensions(EGLDisplay dpy, std::initializer_list<char const*> extensions)
{
    require_extensions(
        extensions,
        [dpy]() -> std::string
        {
            char const* maybe_exts = eglQueryString(dpy, EGL_EXTENSIONS);
            if (maybe_exts)
                return maybe_exts;
            return {};
        });
}

void require_gl_extensions(std::initializer_list<char const*> extensions)
{
    require_extensions(
        extensions,
        []() -> std::string
        {
            char const *maybe_exts =
                reinterpret_cast<char const*>(glGetString(GL_EXTENSIONS));
            if (maybe_exts)
                return maybe_exts;
            return {};
        });
}

bool needs_bounce_buffer(mgm::KMSOutput const& destination, gbm_bo* source)
{
    return destination.buffer_requires_migration(source);
}

const GLchar* const vshader =
    {
        "attribute vec4 position;\n"
        "attribute vec2 texcoord;\n"
        "varying vec2 v_texcoord;\n"
        "void main() {\n"
        "   gl_Position = position;\n"
        "   v_texcoord = texcoord;\n"
        "}\n"
    };

const GLchar* const fshader =
    {
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "uniform sampler2D tex;"
        "varying vec2 v_texcoord;\n"
        "void main() {\n"
        "   gl_FragColor = texture2D(tex, v_texcoord);\n"
        "}\n"
    };

class VBO
{
public:
    VBO(void const* data, size_t size)
    {
        glGenBuffers(1, &buf_id);
        glBindBuffer(GL_ARRAY_BUFFER, buf_id);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    ~VBO()
    {
        glDeleteBuffers(1, &buf_id);
    }

    void bind()
    {
        glBindBuffer(GL_ARRAY_BUFFER, buf_id);
    }

private:
    GLuint buf_id;
};

class EGLBufferCopier
{
public:
    EGLBufferCopier(
        int drm_fd,
        uint32_t width,
        uint32_t height,
        uint32_t format)
        : eglCreateImageKHR{
              reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"))},
          eglDestroyImageKHR{
              reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"))},
          glEGLImageTargetTexture2DOES{
              reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"))},
          device{gbm_create_device(drm_fd), &gbm_device_destroy},
          width{width},
          height{height},
          surface{create_scanout_surface(*device, width, height, format)}
    {
        require_gl_extensions({
            "GL_OES_EGL_image"
        });

        EGLint const config_attr[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 5,
            EGL_GREEN_SIZE, 5,
            EGL_BLUE_SIZE, 5,
            EGL_ALPHA_SIZE, 0,
            EGL_DEPTH_SIZE, 0,
            EGL_STENCIL_SIZE, 0,
            EGL_RENDERABLE_TYPE, MIR_SERVER_EGL_OPENGL_BIT,
            EGL_NONE
        };

        static const EGLint required_egl_version_major = 1;
        static const EGLint required_egl_version_minor = 4;

        EGLint num_egl_configs;
        EGLConfig egl_config;

        display = eglGetDisplay(static_cast<EGLNativeDisplayType>(device.get()));
        if (display == EGL_NO_DISPLAY)
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to get EGL display"));

        EGLint major, minor;

        if (eglInitialize(display, &major, &minor) == EGL_FALSE)
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to initialize EGL display"));

        if ((major < required_egl_version_major) ||
            (major == required_egl_version_major && minor < required_egl_version_minor))
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("Incompatible EGL version"));
        }

        require_egl_extensions(
            display,
            {
                "EGL_KHR_image_base",
                "EGL_EXT_image_dma_buf_import"
            });

        if (eglChooseConfig(display, config_attr, &egl_config, 1, &num_egl_configs) == EGL_FALSE ||
            num_egl_configs != 1)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to choose ARGB EGL config"));
        }

        eglBindAPI(MIR_SERVER_EGL_OPENGL_API);
        static const EGLint context_attr[] = {
#if MIR_SERVER_EGL_OPENGL_BIT == EGL_OPENGL_ES2_BIT
            EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
            EGL_NONE
        };

        context = eglCreateContext(display, egl_config, EGL_NO_CONTEXT, context_attr);
        if (context == EGL_NO_CONTEXT)
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));

        egl_surface = eglCreateWindowSurface(display, egl_config, surface.get(), nullptr);
        eglMakeCurrent(display, egl_surface, egl_surface, context);

        auto vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vshader, nullptr);
        glCompileShader(vertex);

        int compiled;
        glGetShaderiv (vertex, GL_COMPILE_STATUS, &compiled);

        if (!compiled) {
            GLchar log[1024];

            glGetShaderInfoLog (vertex, sizeof log - 1, NULL, log);
            log[sizeof log - 1] = '\0';
            glDeleteShader (vertex);

            BOOST_THROW_EXCEPTION(
                std::runtime_error(std::string{"Failed to compile vertex shader:\n"} + log));
        }


        auto fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fshader, nullptr);
        glCompileShader(fragment);

        glGetShaderiv (fragment, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLchar log[1024];

            glGetShaderInfoLog (fragment, sizeof log - 1, NULL, log);
            log[sizeof log - 1] = '\0';
            glDeleteShader (fragment);

            BOOST_THROW_EXCEPTION(
                std::runtime_error(std::string{"Failed to compile fragment shader:\n"} + log));
        }

        prog = glCreateProgram();
        glAttachShader(prog, vertex);
        glAttachShader(prog, fragment);
        glLinkProgram(prog);
        glGetProgramiv (prog, GL_LINK_STATUS, &compiled);
        if (!compiled) {
            GLchar log[1024];

            glGetProgramInfoLog (prog, sizeof log - 1, NULL, log);
            log[sizeof log - 1] = '\0';

            BOOST_THROW_EXCEPTION(
                std::runtime_error(std::string{"Failed to link shader prog:\n"} + log));
        }

        glUseProgram(prog);

        attrpos = glGetAttribLocation(prog, "position");
        attrtex = glGetAttribLocation(prog, "texcoord");
        auto unitex = glGetUniformLocation(prog, "tex");

        glGenTextures(1, &tex);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);

        glUniform1i(unitex, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        static GLfloat const dest_vert[4][2] =
            { { -1.f, 1.f }, { 1.f, 1.f }, { 1.f, -1.f }, { -1.f, -1.f } };
        vert_data = std::make_unique<VBO>(dest_vert, sizeof(dest_vert));

        static GLfloat const tex_vert[4][2] =
            {
                { 0.f, 0.f }, { 1.f, 0.f }, { 1.f, 1.f }, { 0.f, 1.f },
            };
        tex_data = std::make_unique<VBO>(tex_vert, sizeof(tex_vert));
    }

    EGLBufferCopier(EGLBufferCopier const&) = delete;
    EGLBufferCopier& operator==(EGLBufferCopier const&) = delete;

    ~EGLBufferCopier()
    {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context);
        vert_data = nullptr;
        tex_data = nullptr;
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(display, egl_surface);
        eglDestroyContext(display, context);
        eglTerminate(display);
    }

    mgm::GBMOutputSurface::FrontBuffer copy_front_buffer_from(mgm::GBMOutputSurface::FrontBuffer&& from)
    {
        eglMakeCurrent(display, egl_surface, egl_surface, context);
        mir::Fd const dma_buf{gbm_bo_get_fd(from)};

        glUseProgram(prog);

        EGLint const image_attrs[] = {
            EGL_WIDTH, static_cast<EGLint>(width),
            EGL_HEIGHT, static_cast<EGLint>(height),
            EGL_LINUX_DRM_FOURCC_EXT, DRM_FORMAT_XRGB8888,
            EGL_DMA_BUF_PLANE0_FD_EXT, static_cast<int>(dma_buf),
            EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
            EGL_DMA_BUF_PLANE0_PITCH_EXT, static_cast<EGLint>(gbm_bo_get_stride(from)),
            EGL_NONE
        };

        auto image = eglCreateImageKHR(
            display,
            EGL_NO_CONTEXT,
            EGL_LINUX_DMA_BUF_EXT,
            nullptr,
            image_attrs);

        if (image == EGL_NO_IMAGE_KHR)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGLImage from dma_buf"));
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);

        vert_data->bind();
        glVertexAttribPointer (attrpos, 2, GL_FLOAT, GL_FALSE, 0, 0);

        tex_data->bind();
        glVertexAttribPointer (attrtex, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(attrpos);
        glEnableVertexAttribArray(attrtex);

        GLubyte const idx[] = { 0, 1, 3, 2 };
        glDrawElements (GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

        if (eglSwapBuffers(display, egl_surface) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to swap bounce buffers"));
        }

        eglDestroyImageKHR(display, image);

        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        return mgm::GBMOutputSurface::FrontBuffer(surface.get());
    }

    private:
    static mgm::GBMSurfaceUPtr create_scanout_surface(
        gbm_device& on,
        uint32_t width,
        uint32_t height,
        uint32_t format)
    {
        auto* const device = &on;

        return {
            gbm_surface_create(
                device,
                width,
                height,
                format,
                GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING),
            &gbm_surface_destroy};
    }

    PFNEGLCREATEIMAGEKHRPROC const eglCreateImageKHR;
    PFNEGLDESTROYIMAGEKHRPROC const eglDestroyImageKHR;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC const glEGLImageTargetTexture2DOES;

    std::unique_ptr<gbm_device, decltype(&gbm_device_destroy)> const device;
    uint32_t const width;
    uint32_t const height;
    mgm::GBMSurfaceUPtr const surface;
    EGLDisplay display;
    EGLContext context;
    EGLSurface egl_surface;
    GLuint prog;
    GLuint tex;
    GLint attrtex;
    GLint attrpos;
    std::unique_ptr<VBO> vert_data;
    std::unique_ptr<VBO> tex_data;
};
}

mgm::DisplayBuffer::DisplayBuffer(
    mgm::BypassOption option,
    std::shared_ptr<DisplayReport> const& listener,
    std::vector<std::shared_ptr<KMSOutput>> const& outputs,
    GBMOutputSurface&& surface_gbm,
    geom::Rectangle const& area,
    glm::mat2 const& transformation)
    : listener(listener),
      bypass_option(option),
      outputs(outputs),
      surface{std::move(surface_gbm)},
      area(area),
      transform{transformation},
      needs_set_crtc{false},
      page_flips_pending{false}
{
    listener->report_successful_setup_of_native_resources();

    make_current();

    listener->report_successful_egl_make_current_on_construction();

    glClear(GL_COLOR_BUFFER_BIT);

    surface.swap_buffers();

    listener->report_successful_egl_buffer_swap_on_construction();

    auto temporary_front = surface.lock_front();
    if (!temporary_front)
        fatal_error("Failed to get frontbuffer");

    if (needs_bounce_buffer(*outputs.front(), temporary_front))
    {
        get_front_buffer = std::bind(
            std::mem_fn(&EGLBufferCopier::copy_front_buffer_from),
            std::make_shared<EGLBufferCopier>(
                outputs.front()->drm_fd(),
                surface.size().width.as_int(),
                surface.size().height.as_int(),
                GBM_BO_FORMAT_XRGB8888),
            std::placeholders::_1);
    }
    else
    {
        get_front_buffer = [](auto&& fb) { return std::move(fb); };
    }

    visible_composite_frame = get_front_buffer(std::move(temporary_front));

    /*
     * Check that our (possibly bounced) front buffer is usable on *all* the
     * outputs we've been asked to output on.
     */
    for (auto const& output : outputs)
    {
        if (output->buffer_requires_migration(visible_composite_frame))
        {
            BOOST_THROW_EXCEPTION(std::invalid_argument(
                "Attempted to create a DisplayBuffer spanning multiple GPU memory domains"));
        }
    }

    set_crtc(*outputs.front()->fb_for(visible_composite_frame));

    release_current();

    listener->report_successful_drm_mode_set_crtc_on_construction();
    listener->report_successful_display_construction();
    surface.report_egl_configuration(
        [&listener] (EGLDisplay disp, EGLConfig cfg)
        {
            listener->report_egl_configuration(disp, cfg);
        });
}

mgm::DisplayBuffer::~DisplayBuffer()
{
}

geom::Rectangle mgm::DisplayBuffer::view_area() const
{
    return area;
}

glm::mat2 mgm::DisplayBuffer::transformation() const
{
    return transform;
}

void mgm::DisplayBuffer::set_transformation(glm::mat2 const& t, geometry::Rectangle const& a)
{
    transform = t;
    area = a;
}

bool mgm::DisplayBuffer::overlay(RenderableList const& renderable_list)
{
    glm::mat2 static const no_transformation;
    if (transform == no_transformation &&
       (bypass_option == mgm::BypassOption::allowed))
    {
        mgm::BypassMatch bypass_match(area);
        auto bypass_it = std::find_if(renderable_list.rbegin(), renderable_list.rend(), bypass_match);
        if (bypass_it != renderable_list.rend())
        {
            auto bypass_buffer = (*bypass_it)->buffer();
            auto native = std::dynamic_pointer_cast<mgm::NativeBuffer>(bypass_buffer->native_buffer_handle());
            if (!native)
                BOOST_THROW_EXCEPTION(std::invalid_argument("could not convert NativeBuffer"));
            if (native->flags & mir_buffer_flag_can_scanout &&
                bypass_buffer->size() == surface.size() &&
                !needs_bounce_buffer(*outputs.front(), native->bo))
            {
                if (auto bufobj = outputs.front()->fb_for(native->bo))
                {
                    bypass_buf = bypass_buffer;
                    bypass_bufobj = bufobj;
                    return true;
                }
            }
        }
    }

    bypass_buf = nullptr;
    bypass_bufobj = nullptr;
    return false;
}

void mgm::DisplayBuffer::for_each_display_buffer(
    std::function<void(graphics::DisplayBuffer&)> const& f)
{
    f(*this);
}

void mgm::DisplayBuffer::swap_buffers()
{
    surface.swap_buffers();
    bypass_buf = nullptr;
    bypass_bufobj = nullptr;
}

void mgm::DisplayBuffer::set_crtc(FBHandle const& forced_frame)
{
    for (auto& output : outputs)
    {
        /*
         * Note that failure to set the CRTC is not a fatal error. This can
         * happen under normal conditions when resizing VirtualBox (which
         * actually removes and replaces the virtual output each time so
         * sometimes it's really not there). Xorg often reports similar
         * errors, and it's not fatal.
         */
        if (!output->set_crtc(forced_frame))
            mir::log_error("Failed to set DRM CRTC. "
                "Screen contents may be incomplete. "
                "Try plugging the monitor in again.");
    }
}

void mgm::DisplayBuffer::post()
{
    /*
     * We might not have waited for the previous frame to page flip yet.
     * This is good because it maximizes the time available to spend rendering
     * each frame. Just remember wait_for_page_flip() must be called at some
     * point before the next schedule_page_flip().
     */
    wait_for_page_flip();

    mgm::FBHandle *bufobj;
    if (bypass_buf)
    {
        bufobj = bypass_bufobj;
    }
    else
    {
        scheduled_composite_frame = get_front_buffer(surface.lock_front());
        bufobj = outputs.front()->fb_for(scheduled_composite_frame);
        if (!bufobj)
            fatal_error("Failed to get front buffer object");
    }

    /*
     * Try to schedule a page flip as first preference to avoid tearing.
     * [will complete in a background thread]
     */
    if (!needs_set_crtc && !schedule_page_flip(*bufobj))
        needs_set_crtc = true;

    /*
     * Fallback blitting: Not pretty, since it may tear. VirtualBox seems
     * to need to do this on every frame. [will complete in this thread]
     */
    if (needs_set_crtc)
    {
        set_crtc(*bufobj);
        needs_set_crtc = false;
    }

    using namespace std;  // For operator""ms()

    // Predicted worst case render time for the next frame...
    auto predicted_render_time = 50ms;

    if (bypass_buf)
    {
        /*
         * For composited frames we defer wait_for_page_flip till just before
         * the next frame, but not for bypass frames. Deferring the flip of
         * bypass frames would increase the time we held
         * visible_bypass_frame unacceptably, resulting in client stuttering
         * unless we allocate more buffers (which I'm trying to avoid).
         * Also, bypass does not need the deferred page flip because it has
         * no compositing/rendering step for which to save time for.
         */
        scheduled_bypass_frame = bypass_buf;
        wait_for_page_flip();

        // It's very likely the next frame will be bypassed like this one so
        // we only need time for kernel page flip scheduling...
        predicted_render_time = 5ms;
    }
    else
    {
        /*
         * Not in clone mode? We can afford to wait for the page flip then,
         * making us double-buffered (noticeably less laggy than the triple
         * buffering that clone mode requires).
         */
        if (outputs.size() == 1)
            wait_for_page_flip();

        /*
         * TODO: If you're optimistic about your GPU performance and/or
         *       measure it carefully you may wish to set predicted_render_time
         *       to a lower value here for lower latency.
         *
         *predicted_render_time = 9ms; // e.g. about the same as Weston
         */
    }

    // Buffer lifetimes are managed exclusively by scheduled*/visible* now
    bypass_buf = nullptr;
    bypass_bufobj = nullptr;

    recommend_sleep = 0ms;
    if (outputs.size() == 1)
    {
        auto const& output = outputs.front();
        auto const min_frame_interval = 1000ms / output->max_refresh_rate();
        if (predicted_render_time < min_frame_interval)
            recommend_sleep = min_frame_interval - predicted_render_time;
    }
}

std::chrono::milliseconds mgm::DisplayBuffer::recommended_sleep() const
{
    return recommend_sleep;
}

bool mgm::DisplayBuffer::schedule_page_flip(FBHandle const& bufobj)
{
    /*
     * Schedule the current front buffer object for display. Note that
     * the page flip is asynchronous and synchronized with vertical refresh.
     */
    for (auto& output : outputs)
    {
        if (output->schedule_page_flip(bufobj))
            page_flips_pending = true;
    }

    return page_flips_pending;
}

void mgm::DisplayBuffer::wait_for_page_flip()
{
    if (page_flips_pending)
    {
        for (auto& output : outputs)
            output->wait_for_page_flip();

        page_flips_pending = false;
    }

    if (scheduled_bypass_frame || scheduled_composite_frame)
    {
        // Why are both of these grouped into a single statement?
        // Because in either case both types of frame need releasing each time.

        visible_bypass_frame = scheduled_bypass_frame;
        scheduled_bypass_frame = nullptr;

        visible_composite_frame = std::move(scheduled_composite_frame);
        scheduled_composite_frame = nullptr;
    }
}

void mgm::DisplayBuffer::make_current()
{
    surface.make_current();
}

void mgm::DisplayBuffer::bind()
{
    surface.bind();
}

void mgm::DisplayBuffer::release_current()
{
    surface.release_current();
}

void mgm::DisplayBuffer::schedule_set_crtc()
{
    needs_set_crtc = true;
}

mg::NativeDisplayBuffer* mgm::DisplayBuffer::native_display_buffer()
{
    return this;
}

mgm::GBMOutputSurface::GBMOutputSurface(
    int drm_fd,
    GBMSurfaceUPtr&& surface,
    uint32_t width,
    uint32_t height,
    helpers::EGLHelper&& egl)
    : drm_fd{drm_fd},
      width{width},
      height{height},
      egl{std::move(egl)},
      surface{std::move(surface)}
{
}

mgm::GBMOutputSurface::GBMOutputSurface(GBMOutputSurface&& from)
    : drm_fd{from.drm_fd},
      width{from.width},
      height{from.height},
      egl{std::move(from.egl)},
      surface{std::move(from.surface)}
{
}


void mgm::GBMOutputSurface::make_current()
{
    if (!egl.make_current())
    {
        fatal_error("Failed to make EGL surface current");
    }
}

void mgm::GBMOutputSurface::release_current()
{
    egl.release_current();
}

void mgm::GBMOutputSurface::swap_buffers()
{
    if (!egl.swap_buffers())
        fatal_error("Failed to perform buffer swap");
}

void mgm::GBMOutputSurface::bind()
{

}

auto mgm::GBMOutputSurface::lock_front() -> FrontBuffer
{
    return FrontBuffer{surface.get()};
}

void mgm::GBMOutputSurface::report_egl_configuration(
    std::function<void(EGLDisplay, EGLConfig)> const& to)
{
    egl.report_egl_configuration(to);
}
