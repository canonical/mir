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
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/events/pointer_event.h"
#include "mir_blob.h"

#include <boost/throw_exception.hpp>

MirPointerEvent::MirPointerEvent() :
    MirInputEvent{mir_input_event_type_pointer}
{
}

MirPointerEvent::MirPointerEvent(MirInputDeviceId dev,
                    std::chrono::nanoseconds et,
                    MirInputEventModifiers mods,
                    std::vector<uint8_t> const& cookie,
                    MirPointerAction action,
                    MirPointerButtons buttons,
                    float x,
                    float y,
                    float dx,
                    float dy,
                    float vscroll,
                    float hscroll) :
    MirInputEvent(mir_input_event_type_pointer, dev, et, mods, cookie),
    x_{x},
    y_{y},
    dx_{dx},
    dy_{dy},
    vscroll_{vscroll},
    hscroll_{hscroll},
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
    return x_;
}

void MirPointerEvent::set_x(float x)
{
    x_ = x;
}

float MirPointerEvent::y() const
{
    return y_;
}

void MirPointerEvent::set_y(float y)
{
    y_ = y;
}

float MirPointerEvent::dx() const
{
    return dx_;
}

void MirPointerEvent::set_dx(float dx)
{
    dx_ = dx;
}

float MirPointerEvent::dy() const
{
    return dy_;
}

void MirPointerEvent::set_dy(float dy)
{
    dy_ = dy;
}

float MirPointerEvent::vscroll() const
{
    return vscroll_;
}

void MirPointerEvent::set_vscroll(float vs)
{
    vscroll_ = vs;
}

float MirPointerEvent::hscroll() const
{
    return hscroll_;
}

void MirPointerEvent::set_hscroll(float hs)
{
    hscroll_ = hs;
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
