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

#include "wl_fixes.h"

#include <wayland-server-core.h>

namespace mf = mir::frontend;

namespace
{
class FixesInstance : public mir::wayland::Fixes
{
public:
    explicit FixesInstance(wl_resource* resource)
        : Fixes(resource, Version<1>{})
    {
    }

private:
    void destroy_registry(struct wl_resource* registry) override
    {
        wl_resource_destroy(registry);
    }
};
}

mf::WlFixes::WlFixes(wl_display* display)
    : Global(display, Version<1>{})
{
}

void mf::WlFixes::bind(wl_resource* new_resource)
{
    new FixesInstance{new_resource};
}
