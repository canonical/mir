/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Robert Carr <robert.carr@canonical.com>
 */

#include "mir_test_framework/any_surface.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mtf = mir_test_framework;

namespace 
{
    int width = 783;
    int height = 691;
    MirPixelFormat format = mir_pixel_format_abgr_8888;
}

MirWindow* mtf::make_any_surface(MirConnection *connection)
{
    return mtf::make_surface(connection, mir::geometry::Size{width, height}, format);
}

MirWindow* mtf::make_surface(
    MirConnection *connection, mir::geometry::Size size, MirPixelFormat f)
{
    using namespace std::literals::string_literals;

    auto spec = mir_create_normal_window_spec(connection, size.width.as_int(), size.height.as_int());
    mir_window_spec_set_pixel_format(spec, f);
    auto window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    if (!mir_window_is_valid(window))
    {
        BOOST_THROW_EXCEPTION((
            std::runtime_error{"Failed to create window: "s + mir_window_get_error_message(window)}));
    }
    
    return window;
}
