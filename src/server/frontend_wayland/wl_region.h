/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   William Wold <william.wold@canonical.com>
 */

#ifndef MIR_FRONTEND_WL_REGION_H_
#define MIR_FRONTEND_WL_REGION_H_

#include "generated/wayland_wrapper.h"

#include "mir/geometry/rectangle.h"

#include <vector>

namespace mir
{
namespace frontend
{

class WlRegion: wayland::Region
{
public:
    WlRegion(struct wl_client* client, struct wl_resource* parent, uint32_t id);
    ~WlRegion();

    std::vector<geometry::Rectangle> rectangle_vector();

    static WlRegion* from(wl_resource* resource);

private:
    void destroy() override;
    void add(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void subtract(int32_t x, int32_t y, int32_t width, int32_t height) override;

    std::vector<geometry::Rectangle> rects;
};

}
}

#endif // MIR_FRONTEND_WL_REGION_H_
