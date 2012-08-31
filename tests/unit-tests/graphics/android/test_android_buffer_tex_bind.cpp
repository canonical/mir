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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_test/mock_alloc_adaptor.h"
#include "mir_test/gl_mock.h"

#include <gtest/gtest.h>

namespace mg =mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;
namespace mc=mir::compositor;

class AndroidBufferBinding : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        using namespace testing;
        
        std::shared_ptr<mg::MockAllocAdaptor> mock_alloc_dev(new mg::MockAllocAdaptor);
        EXPECT_CALL(*mock_alloc_dev, alloc_buffer( _, _, _, _, _, _))
            .Times(AtLeast(0));

        buffer = std::shared_ptr<mc::Buffer>(
                            new mga::AndroidBuffer(mock_alloc_dev, geom::Width(300), geom::Height(200), mc::PixelFormat::rgba_8888));
    };

    std::shared_ptr<mc::Buffer> buffer;
    mir::GLMock gl_mock;
};

TEST_F(AndroidBufferBinding, successful_bind)
{

}
