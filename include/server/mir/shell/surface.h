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


#ifndef MIR_SHELL_SURFACE_H_
#define MIR_SHELL_SURFACE_H_

#include "mir/frontend/surface.h"
#include "mir/frontend/surface_id.h"
#include "mir/surfaces/surface.h"
#include "mir/input/surface_target.h"

#include "mir_toolkit/common.h"

#include <string>

namespace mir
{
namespace events
{
class EventSink;
}
namespace frontend
{
struct SurfaceCreationParameters;
}
namespace input
{
class InputChannel;
}

namespace shell
{
class SurfaceBuilder;

class Surface : public frontend::Surface, public input::SurfaceTarget
{
public:
    Surface(
        std::shared_ptr<SurfaceBuilder> const& builder,
        frontend::SurfaceCreationParameters const& params,
        std::shared_ptr<input::InputChannel> const& input_channel);

    Surface(
        std::shared_ptr<SurfaceBuilder> const& builder,
        frontend::SurfaceCreationParameters const& params,
        std::shared_ptr<input::InputChannel> const& input_channel,
        frontend::SurfaceId id,
        std::shared_ptr<events::EventSink> const& sink);

    ~Surface();

    virtual void hide();

    virtual void show();

    virtual void destroy();

    virtual void force_requests_to_complete();

    virtual std::string name() const;

    virtual geometry::Size size() const;
    virtual geometry::Point top_left() const;

    virtual geometry::PixelFormat pixel_format() const;

    virtual void advance_client_buffer();

    virtual std::shared_ptr<compositor::Buffer> client_buffer() const;

    virtual bool supports_input() const;
    virtual int client_input_fd() const;
    virtual int server_input_fd() const;

    virtual int configure(MirSurfaceAttrib attrib, int value);
    virtual MirSurfaceType type() const;
    virtual MirSurfaceState state() const;

private:
    bool set_type(MirSurfaceType t);  // Use configure() to make public changes
    bool set_state(MirSurfaceState s);
    void notify_change(MirSurfaceAttrib attrib, int value);

    std::shared_ptr<SurfaceBuilder> const builder;
    std::shared_ptr<mir::input::InputChannel> const input_channel;
    std::weak_ptr<mir::surfaces::Surface> const surface;

    frontend::SurfaceId const id;
    std::shared_ptr<events::EventSink> const event_sink;

    MirSurfaceType type_value;
    MirSurfaceState state_value;
};
}
}

#endif /* MIR_SHELL_SURFACE_H_ */
