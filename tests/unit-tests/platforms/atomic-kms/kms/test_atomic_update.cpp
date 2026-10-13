/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "atomic_update.h"

#include <mir/graphics/kms/drm_mode_resources.h>
#include <mir/test/death.h>
#include <mir/test/doubles/mock_drm.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cerrno>
#include <csignal>

namespace mga = mir::graphics::atomic;
namespace mgk = mir::graphics::kms;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
constexpr int bogus_fd{7};

class AtomicUpdateTest : public Test
{
public:
    static constexpr uint32_t plane_id{100};
    static constexpr uint32_t framebuffer_property_id{42};
    static constexpr uint64_t framebuffer_id{55};
    static constexpr uint32_t commit_flags{0};
    static constexpr char framebuffer_property_name[]{"FB_ID"};

    AtomicUpdateTest()
    {
        resources.reset();
        resources.add_property(plane_id, DRM_MODE_OBJECT_PLANE, framebuffer_property_id, framebuffer_property_name);

        ON_CALL(mock, drmModeObjectGetProperties(bogus_fd, plane_id, DRM_MODE_OBJECT_PLANE))
            .WillByDefault([this](int, uint32_t id, uint32_t type)
                {
                    return resources.find_object_properties(id, type);
                });
        ON_CALL(mock, drmModeGetProperty(bogus_fd, framebuffer_property_id))
            .WillByDefault([this](int, uint32_t id)
                {
                    return resources.find_property(id);
                });
    }

    auto framebuffer_properties() -> mgk::ObjectProperties
    {
        return {bogus_fd, plane_id, DRM_MODE_OBJECT_PLANE};
    }

    NiceMock<mtd::MockDRM> mock;
    mtd::FakeDRMResources resources;
};
}

TEST_F(AtomicUpdateTest, missing_property_is_fatal)
{
    // Discard the fixture-registered FB_ID, leaving the plane with no properties
    resources.reset();
    auto const properties = framebuffer_properties();

    EXPECT_CALL(mock, drmModeAtomicAddProperty(_, _, _, _)).Times(0);
    EXPECT_CALL(mock, drmModeAtomicCommit(_, _, _, _)).Times(0);
    EXPECT_CALL(mock, drmModeAtomicFree(_)).Times(0);

    MIR_EXPECT_EXIT(
        {
            mga::AtomicUpdate update;
            update.add_property(properties, framebuffer_property_name, framebuffer_id);
        },
        KilledBySignal(SIGABRT),
        "Missing DRM atomic property 'FB_ID' on object ID 100.*"
        "atomic_update.cpp:[0-9]+ in .*AtomicUpdate::add_property.*");
}

TEST_F(AtomicUpdateTest, failed_property_add_is_fatal)
{
    auto const properties = framebuffer_properties();

    ON_CALL(mock, drmModeAtomicAddProperty(_, plane_id, framebuffer_property_id, framebuffer_id))
        .WillByDefault(Return(-EIO));
    EXPECT_CALL(mock, drmModeAtomicCommit(_, _, _, _)).Times(0);
    EXPECT_CALL(mock, drmModeAtomicFree(_)).Times(0);

    MIR_EXPECT_EXIT(
        {
            mga::AtomicUpdate update;
            update.add_property(properties, framebuffer_property_name, framebuffer_id);
        },
        KilledBySignal(SIGABRT),
        "Failed to add DRM atomic property 'FB_ID' \\(property ID 42, object ID 100\\): "
        ".*atomic_update.cpp:[0-9]+ in .*AtomicUpdate::add_property.*");
}

TEST_F(AtomicUpdateTest, allocation_failure_is_fatal)
{
    ON_CALL(mock, drmModeAtomicAlloc()).WillByDefault(Return(nullptr));
    EXPECT_CALL(mock, drmModeAtomicAddProperty(_, _, _, _)).Times(0);
    EXPECT_CALL(mock, drmModeAtomicCommit(_, _, _, _)).Times(0);
    EXPECT_CALL(mock, drmModeAtomicFree(_)).Times(0);

    MIR_EXPECT_EXIT(
        {
            mga::AtomicUpdate update;
        },
        KilledBySignal(SIGABRT),
        "Failed to allocate Atomic DRM update request.*"
        "atomic_update.cpp:[0-9]+ in .*AtomicUpdate::AtomicUpdate.*");
}
