/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_SURFACE_FACTORY_H_
#define MIR_TEST_DOUBLES_STUB_SURFACE_FACTORY_H_

#include <mir/scene/surface_factory.h>
#include <mir/test/doubles/stub_buffer.h>
#include <mir/test/doubles/mock_surface.h>

namespace mir
{
namespace test
{
namespace doubles
{

class StubSurfaceFactory : public scene::SurfaceFactory
{
public:
    std::shared_ptr<scene::Surface> create_surface(
        std::shared_ptr<scene::Session> const&,
        std::list<scene::StreamInfo> const&,
        scene::SurfaceCreationParameters const& params) override
    {
        using namespace testing;
        auto surface = std::make_shared<NiceMock<MockSurface>>();
        ON_CALL(*surface, size()).WillByDefault(Return(params.size));
        return surface;;
    }
};

}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_STUB_SURFACE_FACTORY_H_ */
