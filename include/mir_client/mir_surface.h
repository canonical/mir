/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_SURFACE_H_
#define MIR_SURFACE_H_

#include <string>

namespace mir
{
namespace client
{
namespace detail
{
struct SurfaceState;
}

class Surface
{
public:
    Surface();
    Surface(std::string const& socketfile, int width, int height, int pix_format);

    ~Surface();

    bool is_valid() const;

    Surface(Surface&& that);

    Surface& operator=(Surface&& that);

private:
    detail::SurfaceState* body;
    Surface(Surface const&) = delete;
    Surface& operator=(Surface const&) = delete;
};
}
}

#endif /* MIR_SURFACE_H_ */
