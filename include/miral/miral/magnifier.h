/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIRAL_MAGNIFIER_H
#define MIRAL_MAGNIFIER_H

#include <memory>

namespace mir { class Server; }

namespace miral
{
/// Renders a magnified region of the scene at the cursor position.
/// By default, the magnifier will magnify a 400x300 region below
/// the cursor my a 2x magnitude.
class Magnifier
{
public:
    Magnifier();

    Magnifier& enable(bool enabled);
    Magnifier& magnification(float magnification);
    Magnifier& capture_size(mir::geometry::Size const& size);

    void operator()(mir::Server& server);

private:
    class Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_MAGNIFIER_H
