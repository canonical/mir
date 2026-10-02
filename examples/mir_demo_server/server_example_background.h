/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#ifndef MIR_EXAMPLE_BACKGROUND_H_
#define MIR_EXAMPLE_BACKGROUND_H_

#include <miral/internal_client.h>

#include <memory>

namespace mir
{
class Server;

namespace examples
{

/// An internal client that draws a background on each output, so there is always something to show
class Background
{
public:
    Background();

    void operator()(mir::Server& server);

private:
    struct Self;
    std::shared_ptr<Self> const self;
    miral::StartupInternalClient client;
};

}
}

#endif /* MIR_EXAMPLE_BACKGROUND_H_ */
