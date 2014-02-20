/*
 * Copyright © 2014 Canonical Ltd.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class HwcWrapper : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        mock_device = std::make_shared<testing::NiceMock<mtd::MockHWCComposerDevice1>>();
    }
    std::shared_ptr<mtd::MockHWCComposerDevice1> mock_device;
};

TEST_F(HwcWrapper, uses_commit_and_throws_if_failure)
{
    using namespace testing;

    mga::HwcWrapper device(mock_device, mock_vsync, mock_file_ops);

    EXPECT_CALL(*mock_device, set_interface(mock_device.get(), _, _))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        device.post(*mock_buffer);
    }, std::runtime_error);
}


/* tests with a FRAMEBUFFER_TARGET present
   NOT PORTEDDDDDDDDDDDDDDDDDDDd */
#if 0
//to hwc device adaptor
TEST_F(HwcWrapper, hwc_displays)
{
    using namespace testing;
    EXPECT_CALL(*mock_device, prepare_interface(mock_device.get(),_,_))
        .Times(1);
    EXPECT_CALL(*mock_device, set_interface(mock_device.get(),_,_))
        .Times(1);

<<<<<<< TREE
    mga::HwcWrapperWrapper device(mock_device);

    hwc_display_contents_1_t contents;
    device.prepare(contents);
    device.set(contents);
=======
    mga::HwcWrapper device(mock_device, mock_vsync, mock_file_ops);
    device.render_gl(stub_context);
    device.post(*mock_buffer);
>>>>>>> MERGE-SOURCE

    /* primary phone display */
    EXPECT_TRUE(mock_device->primary_prepare);
    EXPECT_TRUE(mock_device->primary_set);
    /* external monitor display not supported yet */
    EXPECT_FALSE(mock_device->external_prepare);
    EXPECT_FALSE(mock_device->external_set);
    /* virtual monitor display not supported yet */
    EXPECT_FALSE(mock_device->virtual_prepare);
    EXPECT_FALSE(mock_device->virtual_set);
}
#endif




