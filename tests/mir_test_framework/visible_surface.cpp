/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored By: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_test_framework/visible_surface.h"
namespace mtf = mir_test_framework;

mtf::VisibleSurface::VisibleSurface(MirWindowSpec* spec) :
    visible{false}
{
    mir_window_spec_set_event_handler(spec, VisibleSurface::event_callback, this);
    window = mir_create_window_sync(spec);
    // Swap buffers to ensure window is visible for event based tests
    if (mir_window_is_valid(window))
    {
        mir_buffer_stream_swap_buffers_sync(mir_window_get_buffer_stream(window));

        std::unique_lock<std::mutex> lk(mutex);
        if (!cv.wait_for(lk, std::chrono::seconds(5), [this] { return visible; }))
            throw std::runtime_error("timeout waiting for visibility of window");
    }
}

mtf::VisibleSurface::~VisibleSurface()
{
    if (window) mir_window_release_sync(window);
}

void mtf::VisibleSurface::event_callback(MirWindow* surf, MirEvent const* ev, void* context)
{
    if (mir_event_get_type(ev) == mir_event_type_window)
    {
        auto attrib = mir_window_event_get_attribute(mir_event_get_window_event(ev));
        if (attrib == mir_window_attrib_visibility)
        {
            auto ctx = reinterpret_cast<VisibleSurface*>(context);
            ctx->set_visibility(surf, mir_window_event_get_attribute_value(mir_event_get_window_event(ev)));
        }
    }
}

void mtf::VisibleSurface::set_visibility(MirWindow* surf, bool vis)
{
    std::lock_guard<std::mutex> lk(mutex);
    if (surf != window) return;
    visible = vis;
    cv.notify_all();
}

mtf::VisibleSurface::operator MirWindow*() const
{
    return window;
}

mtf::VisibleSurface::VisibleSurface(VisibleSurface&& that) :
    window{that.window}
{
    that.window = nullptr;
}

mtf::VisibleSurface& mtf::VisibleSurface::operator=(VisibleSurface&& that)
{
    std::swap(that.window, window);
    return *this;
}

std::ostream& mtf::operator<<(std::ostream& os, VisibleSurface const& s)
{
    return os << static_cast<MirWindow*>(s);
}
