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

#include "surface_impl.h"
#include "basic_surface.h"
#include "surface_builder.h"
#include "mir/shell/surface_configurator.h"
#include "mir/shell/surface_controller.h"
#include "mir/shell/input_targeter.h"
#include "mir/input/input_channel.h"
#include "mir/frontend/event_sink.h"
#include "mir/frontend/surface_id.h"

#include "mir_toolkit/event.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <cstring>

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mf = mir::frontend;

ms::SurfaceImpl::SurfaceImpl(
    msh::Session* session,
    std::shared_ptr<SurfaceBuilder> const& builder,
    std::shared_ptr<msh::SurfaceConfigurator> const& configurator,
    msh::SurfaceCreationParameters const& params,
    frontend::SurfaceId id,
    std::shared_ptr<mf::EventSink> const& event_sink)
  : builder(builder),
    configurator(configurator),
    surface(builder->create_surface(session, params)),
    id(id),
    event_sink(event_sink),
    type_value(mir_surface_type_normal),
    state_value(mir_surface_state_restored)
{
}

ms::SurfaceImpl::~SurfaceImpl() noexcept
{
    builder->destroy_surface(surface);
}

void ms::SurfaceImpl::hide()
{
    surface->set_hidden(true);
}

void ms::SurfaceImpl::show()
{
    surface->set_hidden(false);
}

void ms::SurfaceImpl::force_requests_to_complete()
{
    surface->force_requests_to_complete();
}

mir::geometry::Size ms::SurfaceImpl::size() const
{
    return surface->size();
}

void ms::SurfaceImpl::move_to(geometry::Point const& p)
{
    surface->move_to(p);
}

mir::geometry::Point ms::SurfaceImpl::top_left() const
{
    return surface->top_left();
}

std::string ms::SurfaceImpl::name() const
{
    return surface->name();
}

mir::geometry::PixelFormat ms::SurfaceImpl::pixel_format() const
{
    return surface->pixel_format();
}

std::shared_ptr<mg::Buffer> ms::SurfaceImpl::advance_client_buffer()
{
    return surface->advance_client_buffer();
}

void ms::SurfaceImpl::allow_framedropping(bool allow)
{
    surface->allow_framedropping(allow);
}
 
void ms::SurfaceImpl::with_most_recent_buffer_do(
    std::function<void(mg::Buffer&)> const& exec)
{
    auto buf = surface->snapshot_buffer();
    exec(*buf);
}

bool ms::SurfaceImpl::supports_input() const
{
    return surface->supports_input();
}

int ms::SurfaceImpl::client_input_fd() const
{
    return surface->client_input_fd();
}

int ms::SurfaceImpl::configure(MirSurfaceAttrib attrib, int value)
{
    int result = 0;
    bool allow_dropping = false;
    /*
     * TODO: In future, query the shell implementation for the subset of
     *       attributes/types it implements.
     */
    value = configurator->select_attribute_value(*this, attrib, value);
    switch (attrib)
    {
    case mir_surface_attrib_type:
        if (!set_type(static_cast<MirSurfaceType>(value)))
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid surface "
                                                   "type."));
        result = type();
        break;
    case mir_surface_attrib_state:
        if (value != mir_surface_state_unknown &&
            !set_state(static_cast<MirSurfaceState>(value)))
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid surface state."));
        result = state();
        break;
    case mir_surface_attrib_focus:
        notify_change(attrib, value);
        break;
    case mir_surface_attrib_swapinterval:
        allow_dropping = (value == 0);
        allow_framedropping(allow_dropping);
        result = value;
        break;
    default:
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid surface "
                                               "attribute."));
        break;
    }

    configurator->attribute_set(*this, attrib, result);

    return result;
}

MirSurfaceType ms::SurfaceImpl::type() const
{
    return type_value;
}

bool ms::SurfaceImpl::set_type(MirSurfaceType t)
{
    bool valid = false;

    if (t >= 0 && t < mir_surface_types)
    {
        type_value = t;
        valid = true;
    }

    return valid;
}

MirSurfaceState ms::SurfaceImpl::state() const
{
    return state_value;
}

bool ms::SurfaceImpl::set_state(MirSurfaceState s)
{
    bool valid = false;

    if (s > mir_surface_state_unknown &&
        s < mir_surface_states)
    {
        state_value = s;
        valid = true;

        notify_change(mir_surface_attrib_state, s);
    }

    return valid;
}

void ms::SurfaceImpl::notify_change(MirSurfaceAttrib attrib, int value)
{
    MirEvent e;

    // This memset is not really required. However it does avoid some
    // harmless uninitialized memory reads that valgrind will complain
    // about, due to gaps in MirEvent.
    memset(&e, 0, sizeof e);

    e.type = mir_event_type_surface;
    e.surface.id = id.as_value();
    e.surface.attrib = attrib;
    e.surface.value = value;

    event_sink->handle_event(e);
}

void ms::SurfaceImpl::take_input_focus(std::shared_ptr<msh::InputTargeter> const& targeter)
{
    targeter->focus_changed(surface->input_channel());
}

void ms::SurfaceImpl::set_input_region(std::vector<geom::Rectangle> const& region)
{
    surface->set_input_region(region);
}

void ms::SurfaceImpl::raise(std::shared_ptr<msh::SurfaceController> const& controller)
{
    controller->raise(surface);
}

void ms::SurfaceImpl::resize(geom::Size const& size)
{
    surface->resize(size);
}

void ms::SurfaceImpl::set_rotation(float degrees, glm::vec3 const& axis)
{
    surface->set_rotation(degrees, axis);
}

void ms::SurfaceImpl::set_alpha(float alpha)
{
    surface->set_alpha(alpha);
}
