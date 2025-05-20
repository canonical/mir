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
class Magnifier
{
public:
    Magnifier();
    ~Magnifier();

    void operator()(mir::Server& server);

private:
    class Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_MAGNIFIER_H
