
/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_WP_VIEWPORTER_H
#define MIR_FRONTEND_WP_VIEWPORTER_H

#include "mir/wayland/weak.h"
#include "viewporter_wrapper.h"
#include "wayland_wrapper.h"

#include "mir/geometry/rectangle.h"

#include <optional>

namespace mir::frontend
{
class WlSurface;

class WpViewporter : public wayland::Viewporter::Global
{
public:
    WpViewporter(wl_display* display);

private:
    void bind(wl_resource* new_wp_viewporter) override;
};

class Viewport : public wayland::Viewport
{
public:
    Viewport(wl_resource* new_viewport, WlSurface* surface);
    ~Viewport() override;

    // Threadsafety: This is a Wayland object, these should only be referenced on the Wayland mainloop
    std::optional<geometry::RectangleD> source;
    std::optional<geometry::Size> destination;

    /**
     * Have the source or destination values changed since the last call to dirty()?
     */
    auto dirty() -> bool;
private:
    void set_source(double x, double y, double width, double height) override;
    void set_destination(int32_t width, int32_t height) override;

    bool dirty_;
    wayland::Weak<wayland::Surface> surface;
};
}

#endif // MIR_FRONTEND_WP_VIEWPORTER_H
