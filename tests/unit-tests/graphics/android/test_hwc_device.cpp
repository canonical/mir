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

#include <gtest/gtest.h>

class HWCDevice : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
    }
};

TEST_F(HWCDevice, test_proc_registration)
{
    hwc_procs_t* procs;
    EXPECT_CALL(*mock_module, registerProcs(mock_module.get(),_))
        .Times(1)
        .WillOnce(SaveArg<1>(&procs));
    HWCDevice device(mock_module);

    EXPECT_NE(nullptr, procs->invalidate);
    EXPECT_NE(nullptr, procs->vsync);
    EXPECT_NE(nullptr, procs->hotplug);

}

#if 0
TEST_F(HWCDevice, test_vsync_activation_comes_after_proc_registration)
{
    testing::InSequence sequence_enforcer; 
    EXPECT_CALL(*mock_module, registerProcs(mock_module.get(),_))
        .Times(1);
    HWCDevice device(mock_module);
    EXPECT_CALL(*mock_module, eventControl(mock_module.get(), 0, HWC_EVENT_VSYNC, 1))
        .Times(1)
        .WillOnce(Return(0));

    HWCDevice device(mock_module);
}

TEST_F(HWCDevice, test_vsync_activation_failure_throws)
{
    HWCDevice device(mock_module);

    EXPECT_CALL(*mock_module, eventControl(mock_module.get(), 0, HWC_EVENT_VSYNC, 1))
        .Times(1)
        .WillOnce(Return(-EINVAL));

    EXPECT_THROW({
        HWCDevice device(mock_module);
    }, std::runtime_error);
}
#endif
