/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_GRAPHICS_NESTED_MIR_API_WRAPPERS_H_
#define MIR_GRAPHICS_NESTED_MIR_API_WRAPPERS_H_

#include "mir_toolkit/mir_client_library.h"

namespace mir
{
namespace graphics
{
namespace nested
{
/// Utilities for making the Mir "toolkit" API more C++ friendly
namespace mir_api_wrappers
{
class MirDisplayConfigHandle
{
public:
    explicit MirDisplayConfigHandle(MirDisplayConfiguration* display_config) :
    display_config{display_config}
    {
    }

    explicit MirDisplayConfigHandle(MirConnection* connection) :
    MirDisplayConfigHandle{mir_connection_create_display_config(connection)}
    {
    }

    ~MirDisplayConfigHandle() noexcept
    {
        mir_display_config_destroy(display_config);
    }

    MirDisplayConfiguration* operator->() const { return display_config; }
    operator MirDisplayConfiguration*() const { return display_config; }

private:
    MirDisplayConfiguration* const display_config;

    MirDisplayConfigHandle(MirDisplayConfigHandle const&) = delete;
    MirDisplayConfigHandle operator=(MirDisplayConfigHandle const&) = delete;
};
}
}
}
}

#endif /* MIR_GRAPHICS_NESTED_MIR_API_WRAPPERS_H_ */
