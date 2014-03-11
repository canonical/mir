/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_NULL_SCREENCAST_H_
#define MIR_TEST_DOUBLES_NULL_SCREENCAST_H_

#include "mir/frontend/screencast.h"

namespace mir
{
namespace test
{
namespace doubles
{

class NullScreencast : public frontend::Screencast
{
public:
    frontend::ScreencastSessionId create_session(
        geometry::Rectangle const&,
        geometry::Size const&,
        MirPixelFormat)
    {
        return frontend::ScreencastSessionId{1};
    }

    void destroy_session(frontend::ScreencastSessionId) {}

    std::shared_ptr<graphics::Buffer> capture(frontend::ScreencastSessionId)
    {
        return nullptr;
    }
};

}
}
}

#endif /* MIR_TEST_DOUBLES_NULL_SCREENCAST_H_ */
