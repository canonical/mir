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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_HOST_CONNECTION_H_
#define MIR_TEST_DOUBLES_STUB_HOST_CONNECTION_H_

#include "src/server/graphics/nested/host_connection.h"

namespace mir
{
namespace test
{
namespace doubles
{

class StubHostConnection : public graphics::nested::HostConnection
{
public:
    std::vector<int> platform_fd_items() override { return {}; }

    EGLNativeDisplayType egl_native_display() override { return {}; }

    std::shared_ptr<MirDisplayConfiguration> create_display_config() override
    {
        return std::shared_ptr<MirDisplayConfiguration>{
            new MirDisplayConfiguration{0, nullptr, 0, nullptr}};
    }

    void set_display_config_change_callback(std::function<void()> const&) override
    {
    }

    void apply_display_config(MirDisplayConfiguration&) override {}

    std::shared_ptr<graphics::nested::HostSurface>
        create_surface(MirSurfaceParameters const&) override
    {
        class NullHostSurface : public graphics::nested::HostSurface
        {
        public:
            EGLNativeWindowType egl_native_window() override { return {}; }
            void set_event_handler(MirEventDelegate const*) override {}
        };

        return std::make_shared<NullHostSurface>();
    }

    void drm_auth_magic(int) override {}
    void drm_set_gbm_device(struct gbm_device*) override {}
};


}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_HOST_CONNECTION_H_ */
