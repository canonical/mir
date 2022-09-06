/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/events/pointer_event.h"
#include "mir_blob.h"

#include <boost/throw_exception.hpp>

namespace geom = mir::geometry;
namespace mev = mir::events;

MirPointerEvent::MirPointerEvent() :
    MirInputEvent{mir_input_event_type_pointer}
{
}

MirPointerEvent::MirPointerEvent(MirInputDeviceId dev,
                    std::chrono::nanoseconds et,
                    std::vector<uint8_t> const& cookie,
                    MirInputEventModifiers mods,
                    MirPointerAction action,
                    MirPointerButtons buttons,
                    std::optional<geom::PointF> position,
                    geom::DisplacementF motion,
                    MirPointerAxisSource axis_source,
                    mev::ScrollAxisH h_scroll,
                    mev::ScrollAxisV v_scroll)
    : MirInputEvent(mir_input_event_type_pointer, dev, et, mods, cookie),
      position_{position},
      motion_{motion},
      axis_source_{axis_source},
      h_scroll_{h_scroll},
      v_scroll_{v_scroll},
      action_{action},
      buttons_{buttons}
{
}

auto MirPointerEvent::clone() const -> MirPointerEvent*
{
    return new MirPointerEvent{*this};
}

MirPointerButtons MirPointerEvent::buttons() const
{
    return buttons_;
}

void MirPointerEvent::set_buttons(MirPointerButtons buttons)
{
    buttons_ = buttons;
}

auto MirPointerEvent::position() const -> std::optional<mir::geometry::PointF>
{
    return position_;
}

void MirPointerEvent::set_position(std::optional<mir::geometry::PointF> value)
{
    position_ = value;
}

auto MirPointerEvent::local_position() const -> std::optional<mir::geometry::PointF>
{
    return local_position_;
}

void MirPointerEvent::set_local_position(std::optional<mir::geometry::PointF> value)
{
    local_position_ = value;
}

auto MirPointerEvent::motion() const -> mir::geometry::DisplacementF
{
    return motion_;
}

void MirPointerEvent::set_motion(mir::geometry::DisplacementF value)
{
    motion_ = value;
}

auto MirPointerEvent::h_scroll() const -> mev::ScrollAxisH
{
    return h_scroll_;
}

void MirPointerEvent::set_h_scroll(mev::ScrollAxisH value)
{
    h_scroll_ = value;
}

auto MirPointerEvent::v_scroll() const -> mev::ScrollAxisV
{
    return v_scroll_;
}

void MirPointerEvent::set_v_scroll(mev::ScrollAxisV value)
{
    v_scroll_ = value;
}

MirPointerAction MirPointerEvent::action() const
{
    return action_;
}

void MirPointerEvent::set_action(MirPointerAction action)
{
    action_ = action;
}

void MirPointerEvent::set_dnd_handle(std::vector<uint8_t> const& handle)
{
    dnd_handle_ = handle;
}

namespace
{
struct MyMirBlob : MirBlob
{

    size_t size() const override { return data_.size(); }
    virtual void const* data() const override { return data_.data(); }

    std::vector<uint8_t> data_;
};
}

MirBlob* MirPointerEvent::dnd_handle() const
{
    if (!dnd_handle_)
        return nullptr;

    auto const dnd_handle = *dnd_handle_;

    auto blob = std::make_unique<MyMirBlob>();
    blob->data_.reserve(dnd_handle.size());

    // Can't use std::copy() as the CapnP iterators don't provide an iterator category
    for (auto p = dnd_handle.begin(); p != dnd_handle.end(); ++p)
        blob->data_.push_back(*p);

    return blob.release();
}

auto MirPointerEvent::axis_source() const -> MirPointerAxisSource
{
    return axis_source_;
}

void MirPointerEvent::set_axis_source(MirPointerAxisSource source)
{
    axis_source_ = source;
}
