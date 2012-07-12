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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/compositor/buffer_bundle.h"
#include "mir/compositor/buffer_bundle_factory.h"
#include "mir/surfaces/surface_stack.h"
#include "mir/surfaces/surface.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

namespace mc = mir::compositor;
namespace ms = mir::surfaces;
namespace mg = mir::geometry;

#if 0
namespace
{
struct MockBufferBundleFactory : public mc::BufferBundleFactory
{
    MOCK_METHOD3(
        create_buffer_bundle,
        std::shared_ptr<mc::BufferBundle>(
            mg::Width width,
            mg::Height height,
            mc::PixelFormat pf));

};

}

TEST(
    TestSurfaceStack,
    surface_creation_destruction_creates_and_destroys_buffer_bundle)
{
    using namespace ::testing;

    std::unique_ptr<mc::BufferSwapper> swapper_handle(nullptr);
    mc::BufferBundle buffer_bundle(std::move(swapper_handle));
    MockBufferBundleFactory buffer_bundle_factory;
/*
    EXPECT_CALL(
        buffer_bundle_factory,
        create_buffer_bundle(_, _, _))
            .Times(AtLeast(1))
            .WillRepeatedly(Return(std::shared_ptr<mc::BufferBundle>(new mc::BufferBundle(std::move(swapper_handle)))));
*/
     
    ms::SurfaceStack stack(&buffer_bundle_factory);
    std::weak_ptr<ms::Surface> surface = stack.create_surface(
        ms::a_surface().of_size(mg::Width(1024), mg::Height(768)));
    
    stack.destroy_surface(surface);
}
#endif
