/*
 * Copyright © 2017 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef TESTS_ACCEPTANCE_TESTS_CLIENT_SCREENCAST_H_
#define TESTS_ACCEPTANCE_TESTS_CLIENT_SCREENCAST_H_

#include "mir_test_framework/connected_client_with_a_surface.h"
#include "mir/test/doubles/stub_session_authorizer.h"

namespace mir
{
namespace test
{
struct ScreencastBase : mir_test_framework::HeadlessInProcessServer
{
    void SetUp() override;
    MirScreencastSpec* create_default_screencast_spec(MirConnection* connection);
    doubles::MockSessionAuthorizer mock_authorizer;
    unsigned int const default_width{1};
    unsigned int const default_height{1};
    MirPixelFormat const default_pixel_format{mir_pixel_format_abgr_8888};
    MirRectangle const default_capture_region{0, 0, 1, 1};
};
}
}

#endif /* TESTS_ACCEPTANCE_TESTS_CLIENT_SCREENCAST_H_ */
