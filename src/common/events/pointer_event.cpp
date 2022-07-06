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

float MirPointerEvent::x() const
{
    return position_.value_or(geom::PointF{}).x.as_value();
}

void MirPointerEvent::set_x(float x)
{
    if (!position_)
    {
        position_.emplace();
    }
    position_.value().x = geom::XF{x};
}

float MirPointerEvent::y() const
{
    return position_.value_or(geom::PointF{}).y.as_value();
}

void MirPointerEvent::set_y(float y)
{
    if (!position_)
    {
        position_.emplace();
    }
    position_.value().y = geom::YF{y};
}

bool MirPointerEvent::has_absolute_position() const
{
    return position_.operator bool();
}

void MirPointerEvent::set_has_absolute_position(bool value)
{
    if (value && !position_)
    {
        position_ = geom::PointF{};
    }
    else if (!value && position_)
    {
        position_ = std::nullopt;
    }
}

float MirPointerEvent::dx() const
{
    return motion_.dx.as_value();
}

void MirPointerEvent::set_dx(float dx)
{
    motion_.dx = geom::DeltaXF{dx};
}

float MirPointerEvent::dy() const
{
    return motion_.dy.as_value();
}

void MirPointerEvent::set_dy(float dy)
{
    motion_.dy = geom::DeltaYF{dy};
}

float MirPointerEvent::vscroll() const
{
    return scroll_.dy.as_value();
}

void MirPointerEvent::set_vscroll(float vs)
{
    scroll_.dy = geom::DeltaYF{vs};
}

float MirPointerEvent::hscroll() const
{
    return scroll_.dx.as_value();
}

void MirPointerEvent::set_hscroll(float hs)
{
    scroll_.dx = geom::DeltaXF{hs};
}

bool MirPointerEvent::vscroll_stop() const
{
    return scroll_stop_.dy.as_value();
}

void MirPointerEvent::set_vscroll_stop(bool stop)
{
    scroll_stop_.dy = geom::generic::DeltaY<bool>{stop};
}

bool MirPointerEvent::hscroll_stop() const
{
    return scroll_stop_.dx.as_value();
}

void MirPointerEvent::set_hscroll_stop(bool stop)
{
    scroll_stop_.dx = geom::generic::DeltaX<bool>{stop};
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

float MirPointerEvent::vscroll_discrete() const
{
    return scroll_discrete_.dy.as_value();
}

void MirPointerEvent::set_vscroll_discrete(float v)
{
    scroll_discrete_.dy = geom::DeltaY{v};
}

float MirPointerEvent::hscroll_discrete() const
{
    return scroll_discrete_.dx.as_value();
}

void MirPointerEvent::set_hscroll_discrete(float h)
{
    scroll_discrete_.dx = geom::DeltaX{h};
}
