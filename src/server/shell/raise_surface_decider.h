/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include <memory>

#include "mir_toolkit/cookie.h"

namespace mir
{
namespace cookie
{
class CookieFactory;
}
namespace scene
{
class Surface;
}
namespace shell
{

class RaiseSurfaceDecider
{
public:
    RaiseSurfaceDecider(std::shared_ptr<cookie::CookieFactory> const& cookie_factory);

    bool should_raise_surface(
        std::shared_ptr<scene::Surface> const& focused_surface,
        MirCookie const& cookie) const;

private:
    std::shared_ptr<cookie::CookieFactory> const cookie_factory;
};

}
}
