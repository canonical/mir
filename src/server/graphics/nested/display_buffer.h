/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_GRAPHICS_NESTED_DETAIL_NESTED_OUTPUT_H_
#define MIR_GRAPHICS_NESTED_DETAIL_NESTED_OUTPUT_H_

#include "mir/graphics/display_buffer.h"
#include "mir/renderer/gl/render_target.h"
#include "display.h"
#include "host_surface.h"
#include "passthrough_option.h"
#include "host_chain.h"

#include <map>
#include <glm/glm.hpp>
#include <EGL/egl.h>

namespace mir
{
namespace graphics
{
namespace nested
{
class HostSurface;
class HostStream;
class Buffer;
namespace detail
{

class DisplayBuffer : public graphics::DisplayBuffer,
                      public graphics::NativeDisplayBuffer,
                      public renderer::gl::RenderTarget
{
public:
    DisplayBuffer(
        EGLDisplayHandle const& egl_display,
        DisplayConfigurationOutput best_output,
        std::shared_ptr<HostConnection> const& host_connection,
        PassthroughOption option);

    ~DisplayBuffer() noexcept;

    geometry::Rectangle view_area() const override;
    void make_current() override;
    void release_current() override;
    void swap_buffers() override;
    void bind() override;
    MirOrientation orientation() const override;
    MirMirrorMode mirror_mode() const override;

    bool overlay(RenderableList const& renderlist) override;

    NativeDisplayBuffer* native_display_buffer() override;

    DisplayBuffer(DisplayBuffer const&) = delete;
    DisplayBuffer operator=(DisplayBuffer const&) = delete;
private:
    EGLDisplayHandle const& egl_display;
    std::shared_ptr<HostStream> const host_stream;
    std::shared_ptr<HostSurface> const host_surface;
    std::shared_ptr<HostConnection> const host_connection;
    std::unique_ptr<HostChain> host_chain;
    EGLConfig const egl_config;
    EGLContextStore const egl_context;
    geometry::Rectangle const area;
    EGLSurfaceHandle const egl_surface;
    PassthroughOption const passthrough_option;

    static void event_thunk(MirWindow* surface, MirEvent const* event, void* context);
    void mir_event(MirEvent const& event);

    enum class BackingContent
    {
        stream,
        chain
    } content;
    glm::mat4 const identity;

    std::mutex mutex;
    typedef std::tuple<MirBuffer*, MirPresentationChain*> SubmissionInfo;
    std::map<SubmissionInfo, std::shared_ptr<graphics::Buffer>> submitted_buffers;
    SubmissionInfo last_submitted { nullptr, nullptr };

    void release_buffer(MirBuffer* b, MirPresentationChain* c);
};
}
}
}
}

#endif /* MIR_GRAPHICS_NESTED_DETAIL_NESTED_OUTPUT_H_ */
