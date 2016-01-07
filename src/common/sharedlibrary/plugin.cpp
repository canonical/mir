/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/plugin.h"
#include "mir/fatal.h"

namespace mir
{

Plugin::Plugin()
{
}

Plugin::~Plugin() noexcept
{
    // This is not a recoverable error. If we don't exit now, we'll just
    // end up with a worse crash where we have unloaded the library we're
    // executing in, and so get no stack trace at all...
    if (library.unique())
        mir::fatal_error_abort("Attempted to unload a plugin library "
                               "while it's still in use (~Plugin not "
                               "completed yet)");
}

void Plugin::keep_library_loaded(std::shared_ptr<void> const& lib)
{
    library = lib;
}

std::shared_ptr<void> Plugin::keep_library_loaded() const
{
    return library;
}

} // namespace mir
