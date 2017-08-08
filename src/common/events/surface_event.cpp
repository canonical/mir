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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "mir/events/surface_event.h"
#include "mir_blob.h"

// MirSurfaceEvent is a deprecated type, but we need to implement it
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

MirSurfaceEvent::MirSurfaceEvent()
{
    event.initSurface();
}

int MirSurfaceEvent::id() const
{
    return event.asReader().getSurface().getId();
}

void MirSurfaceEvent::set_id(int id)
{
    event.getSurface().setId(id);
}

MirWindowAttrib MirSurfaceEvent::attrib() const
{
    return static_cast<MirWindowAttrib>(event.asReader().getSurface().getAttrib());
}

void MirSurfaceEvent::set_attrib(MirWindowAttrib attrib)
{
    event.getSurface().setAttrib(static_cast<mir::capnp::SurfaceEvent::Attrib>(attrib));
}

int MirSurfaceEvent::value() const
{
    return event.asReader().getSurface().getValue();
}

void MirSurfaceEvent::set_value(int value)
{
    event.getSurface().setValue(value);
}

void MirSurfaceEvent::set_dnd_handle(std::vector<uint8_t> const& handle)
{
    event.getSurface().initDndHandle(handle.size());
    event.getSurface().setDndHandle(::kj::ArrayPtr<uint8_t const>{&*begin(handle), &*end(handle)});
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

MirBlob* MirSurfaceEvent::dnd_handle() const
{
    if (!event.asReader().getSurface().hasDndHandle())
        return nullptr;

    auto blob = std::make_unique<MyMirBlob>();

    auto reader = event.asReader().getSurface().getDndHandle();

    blob->data_.reserve(reader.size());

    // Can't use std::copy() as the CapnP iterators don't provide an iterator category
    for (auto p = reader.begin(); p != reader.end(); ++p)
        blob->data_.push_back(*p);

    return blob.release();
}

