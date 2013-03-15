/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_CLIENT_SURFACE_H_
#define MIR_TEST_DOUBLES_MOCK_CLIENT_SURFACE_H_

#include "src/client/mir_client_surface.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockClientSurface : public client::ClientSurface
{
    MOCK_CONST_METHOD0(get_parameters, MirSurfaceParameters());
    MOCK_METHOD0(get_current_buffer, std::shared_ptr<client::ClientBuffer>());
    MOCK_METHOD2(next_buffer, MirWaitHandle*(mir_surface_lifecycle_callback, void*));
};

}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_CLIENT_SURFACE_H_ */
