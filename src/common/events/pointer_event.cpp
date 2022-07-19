/*
 * Copyright © 2016-2017 Canonical Ltd.
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
                    geom::DisplacementF scroll,
                    geom::Displacement scroll_discrete,
                    geom::generic::Displacement<geom::generic::Value<bool>::Wrapper> scroll_stop)
    : MirInputEvent(mir_input_event_type_pointer, dev, et, mods, cookie),
      position_{position},
      motion_{motion},
      axis_source_{axis_source},
      scroll_{scroll},
      scroll_discrete_{scroll_discrete},
      scroll_stop_{scroll_stop},
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

auto MirPointerEvent::motion() const -> mir::geometry::DisplacementF
{
    return motion_;
}

void MirPointerEvent::set_motion(mir::geometry::DisplacementF value)
{
    motion_ = value;
}

auto MirPointerEvent::scroll() const -> mir::geometry::DisplacementF
{
    return scroll_;
}

void MirPointerEvent::set_scroll(mir::geometry::DisplacementF value)
{
    scroll_ = value;
}

auto MirPointerEvent::scroll_discrete() const -> mir::geometry::Displacement
{
    return scroll_discrete_;
}

void MirPointerEvent::set_scroll_discrete(mir::geometry::Displacement value)
{
    scroll_discrete_ = value;
}

auto MirPointerEvent::scroll_stop() const -> mir::geometry::generic::Displacement<mir::geometry::generic::Value<bool>::Wrapper>
{
    return scroll_stop_;
}

void MirPointerEvent::set_scroll_stop(mir::geometry::generic::Displacement<mir::geometry::generic::Value<bool>::Wrapper> value)
{
    scroll_stop_ = value;
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
