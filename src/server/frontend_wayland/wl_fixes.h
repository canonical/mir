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

#ifndef MIR_FRONTEND_WL_FIXES_H
#define MIR_FRONTEND_WL_FIXES_H

#include "wayland_wrapper.h"

namespace mir::frontend
{
class WlFixes : public wayland::Fixes::Global
{
public:
    explicit WlFixes(wl_display* display);

private:
    void bind(wl_resource* new_wl_fixes) override;
};
}

#endif // MIR_FRONTEND_WL_FIXES_H
