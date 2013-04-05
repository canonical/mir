/*
 * Copyright © 2013 Canonical Ltd.
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

#include "src/server/graphics/android/fb_swapper.h"
#include "mir_test_doubles/mock_buffer.h"

#include <future>
#include <thread>
#include <stdexcept>
#include <gtest/gtest.h>

namespace mc=mir::compositor;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;

class FBSwapperTest : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        buffer1 = std::make_shared<mtd::MockBuffer>();
        buffer2 = std::make_shared<mtd::MockBuffer>();
        buffer3 = std::make_shared<mtd::MockBuffer>();
    }

    std::shared_ptr<mc::Buffer> buffer1;
    std::shared_ptr<mc::Buffer> buffer2;
    std::shared_ptr<mc::Buffer> buffer3;
};

TEST_F(FBSwapperTest, simple_swaps_returns_valid)
{
    mga::FBSwapper fb_swapper({buffer1, buffer2});

    auto test_buffer = fb_swapper.compositor_acquire();

    EXPECT_TRUE((test_buffer == buffer1) || (test_buffer == buffer2));
} 

TEST_F(FBSwapperTest, simple_swaps_return_aba_pattern)
{
    mga::FBSwapper fb_swapper({buffer1, buffer2});

    auto test_buffer_1 = fb_swapper.compositor_acquire();
    fb_swapper.compositor_release(test_buffer_1);

    auto test_buffer_2 = fb_swapper.compositor_acquire();
    fb_swapper.compositor_release(test_buffer_2);

    auto test_buffer_3 = fb_swapper.compositor_acquire();
    fb_swapper.compositor_release(test_buffer_3);

    EXPECT_EQ(test_buffer_1, test_buffer_3);
    EXPECT_NE(test_buffer_1, test_buffer_2);
} 

TEST_F(FBSwapperTest, triple_swaps_return_abcab_pattern)
{
    mga::FBSwapper fb_swapper({buffer1, buffer2, buffer3});

    auto test_buffer_1 = fb_swapper.compositor_acquire();
    fb_swapper.compositor_release(test_buffer_1);

    auto test_buffer_2 = fb_swapper.compositor_acquire();
    fb_swapper.compositor_release(test_buffer_2);

    auto test_buffer_3 = fb_swapper.compositor_acquire();
    fb_swapper.compositor_release(test_buffer_3);

    auto test_buffer_4 = fb_swapper.compositor_acquire();
    fb_swapper.compositor_release(test_buffer_4);

    auto test_buffer_5 = fb_swapper.compositor_acquire();
    fb_swapper.compositor_release(test_buffer_5);

    EXPECT_EQ(test_buffer_1, test_buffer_4);
    EXPECT_NE(test_buffer_1, test_buffer_2);
    EXPECT_NE(test_buffer_1, test_buffer_3);

    EXPECT_EQ(test_buffer_2, test_buffer_5);
    EXPECT_NE(test_buffer_2, test_buffer_1);
    EXPECT_NE(test_buffer_2, test_buffer_3);
}

//not supported yet. we will support this for composition bypass
TEST_F(FBSwapperTest, client_acquires_throw)
{
    mga::FBSwapper fb_swapper({buffer1, buffer2});

    EXPECT_THROW({
        fb_swapper.client_acquire();
    }, std::runtime_error);

    EXPECT_THROW({
        std::shared_ptr<mc::Buffer> empty;
        fb_swapper.client_release(empty);
    }, std::runtime_error);
}

TEST_F(FBSwapperTest, synctest)
{
    std::vector<std::shared_ptr<mc::Buffer>> test_buffers{buffer1, buffer2};
    mga::FBSwapper fb_swapper(test_buffers);

    std::vector<std::shared_ptr<mc::Buffer>> blist;
    std::mutex mut;
    for(auto i=0u; i < 150; ++i)
    {
        auto buf1 = fb_swapper.compositor_acquire();
        auto buf2 = fb_swapper.compositor_acquire();

        auto f = std::async(std::launch::async,
                            [&]()
                            {
                                /* driver will release in order */
                                fb_swapper.compositor_release(buf1);
                                fb_swapper.compositor_release(buf2);
                            });

        //this line will wait if f has not run
        auto buf3 = fb_swapper.compositor_acquire();
        f.wait(); //make sure buf3 is released after buf2 to maintain order
        fb_swapper.compositor_release(buf3);

        blist.push_back(buf1);
        blist.push_back(buf2);
        blist.push_back(buf3);
    }

    //This chunk of code makes sure "ABABA..." or "BABAB..." pattern was produced 
    auto modcounter = 0u;
    for(auto i : test_buffers)
    {
        if (blist[0] == i)
        {
            break;
        }
        modcounter++;
    }

    for(auto i : blist)
    {
        EXPECT_EQ(test_buffers[(modcounter % test_buffers.size())], i);
        modcounter++;
    }
}
