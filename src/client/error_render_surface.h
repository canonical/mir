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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_CLIENT_ERROR_RENDER_SURFACE_H_
#define MIR_CLIENT_ERROR_RENDER_SURFACE_H_

#include "mir/mir_render_surface.h"

#include <string>

namespace mir
{
namespace client
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
class ErrorRenderSurface : public MirRenderSurface
{
public:
    ErrorRenderSurface(
        std::string const& error_msg, MirConnection* conn);

    MirConnection* connection() const override;
    mir::frontend::BufferStreamId stream_id() const override;
    mir::geometry::Size size() const override;
    void set_size(mir::geometry::Size) override;
    bool valid() const override;
    char const* get_error_message() const override;
    MirBufferStream* get_buffer_stream(
        int width, int height,
        MirPixelFormat format,
        MirBufferUsage buffer_usage) override;
    MirPresentationChain* get_presentation_chain() override;
private:
    std::string const error;
    MirConnection* const connection_;
};
#pragma GCC diagnostic pop
}
}
#endif
