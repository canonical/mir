/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
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
#include <mir/test/doubles/mock_drm.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

#include <cerrno>

namespace mga = mir::graphics::atomic;
namespace mgk = mir::graphics::kms;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
using AtomicProperty = mtd::MockDRM::AtomicProperty;
using AtomicCommit = mtd::MockDRM::AtomicCommit;
using AtomicOperation = mtd::MockDRM::AtomicOperation;
using AtomicOperationKind = mtd::MockDRM::AtomicOperationKind;

constexpr int bogus_fd{7};
constexpr size_t first_request_id{1};
constexpr size_t second_request_id{2};
constexpr uint32_t plane_id{10};
constexpr uint32_t connector_id{11};
constexpr uint32_t crtc_id{12};
constexpr uint32_t framebuffer_property_id{20};
constexpr uint32_t connector_crtc_property_id{21};
constexpr uint32_t mode_property_id{22};
constexpr uint64_t initial_framebuffer_id{30};
constexpr uint64_t assigned_crtc_id{31};
constexpr uint64_t mode_blob_id{32};
constexpr uint64_t replacement_framebuffer_id{40};
constexpr AtomicProperty framebuffer_property{plane_id, framebuffer_property_id, initial_framebuffer_id};
constexpr AtomicProperty connector_assignment{connector_id, connector_crtc_property_id, assigned_crtc_id};
constexpr AtomicProperty mode_property{crtc_id, mode_property_id, mode_blob_id};
constexpr AtomicProperty replacement_framebuffer{plane_id, framebuffer_property_id, replacement_framebuffer_id};

class MockDRMAtomicRecordingTest : public Test
{
public:
    NiceMock<mtd::MockDRM> mock;
};

class AtomicUpdateTest : public MockDRMAtomicRecordingTest
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

    mtd::FakeDRMResources resources;
};
}

TEST_F(MockDRMAtomicRecordingTest, test_only_commits_preserve_visible_state_and_record_immutable_snapshots)
{
    auto const request = drmModeAtomicAlloc();
    ASSERT_NE(request, nullptr);

    EXPECT_EQ(drmModeAtomicAddProperty(request, plane_id, framebuffer_property_id, initial_framebuffer_id), 1);
    EXPECT_EQ(drmModeAtomicAddProperty(request, connector_id, connector_crtc_property_id, assigned_crtc_id), 2);
    EXPECT_EQ(drmModeAtomicCommit(bogus_fd, request, DRM_MODE_ATOMIC_TEST_ONLY, nullptr), 0);
    EXPECT_TRUE(mock.atomic_visible_properties().empty());

    EXPECT_EQ(drmModeAtomicCommit(bogus_fd, request, DRM_MODE_ATOMIC_ALLOW_MODESET, nullptr), 0);
    EXPECT_EQ(mock.atomic_visible_properties().at({plane_id, framebuffer_property_id}), initial_framebuffer_id);
    EXPECT_EQ(mock.atomic_visible_properties().at({connector_id, connector_crtc_property_id}), assigned_crtc_id);
    EXPECT_EQ(drmModeAtomicAddProperty(request, plane_id, framebuffer_property_id, replacement_framebuffer_id), 3);
    EXPECT_EQ(drmModeAtomicCommit(bogus_fd, request, DRM_MODE_ATOMIC_TEST_ONLY, nullptr), 0);
    EXPECT_EQ(mock.atomic_visible_properties().at({plane_id, framebuffer_property_id}), initial_framebuffer_id);
    drmModeAtomicFree(request);

    EXPECT_THAT(mock.atomic_requests(), ElementsAre(mtd::FreedAtomicRequest(first_request_id)));
    EXPECT_THAT(mock.atomic_commits(), ElementsAre(
        AtomicCommit{first_request_id, bogus_fd, DRM_MODE_ATOMIC_TEST_ONLY, 0,
                     {framebuffer_property, connector_assignment}},
        AtomicCommit{first_request_id, bogus_fd, DRM_MODE_ATOMIC_ALLOW_MODESET, 0,
                     {framebuffer_property, connector_assignment}},
        AtomicCommit{first_request_id, bogus_fd, DRM_MODE_ATOMIC_TEST_ONLY, 0,
                     {framebuffer_property, connector_assignment, replacement_framebuffer}}));
    EXPECT_THAT(mock.atomic_operations(), ElementsAre(
        AtomicOperation{AtomicOperationKind::allocate, first_request_id, 0},
        AtomicOperation{AtomicOperationKind::add_property, first_request_id, 1},
        AtomicOperation{AtomicOperationKind::add_property, first_request_id, 2},
        AtomicOperation{AtomicOperationKind::commit, first_request_id, 0},
        AtomicOperation{AtomicOperationKind::commit, first_request_id, 0},
        AtomicOperation{AtomicOperationKind::add_property, first_request_id, 3},
        AtomicOperation{AtomicOperationKind::commit, first_request_id, 0},
        AtomicOperation{AtomicOperationKind::free, first_request_id, 0}));
}

TEST_F(MockDRMAtomicRecordingTest, independently_records_test_only_and_real_commit_failures_from_gmock)
{
    auto const request = drmModeAtomicAlloc();
    ASSERT_NE(request, nullptr);
    EXPECT_EQ(drmModeAtomicAddProperty(request, plane_id, framebuffer_property_id, initial_framebuffer_id), 1);

    EXPECT_CALL(mock, drmModeAtomicCommit(bogus_fd, request, _, nullptr))
        .WillOnce(Return(-EIO))
        .WillOnce(Return(0))
        .WillOnce(Return(-EBUSY))
        .WillOnce(Return(0));
    EXPECT_EQ(drmModeAtomicCommit(bogus_fd, request, DRM_MODE_ATOMIC_TEST_ONLY, nullptr), -EIO);
    EXPECT_TRUE(mock.atomic_visible_properties().empty());
    EXPECT_EQ(drmModeAtomicCommit(bogus_fd, request, 0, nullptr), 0);
    EXPECT_EQ(mock.atomic_visible_properties().at({plane_id, framebuffer_property_id}), initial_framebuffer_id);

    EXPECT_EQ(drmModeAtomicAddProperty(request, plane_id, framebuffer_property_id, replacement_framebuffer_id), 2);
    EXPECT_EQ(drmModeAtomicCommit(bogus_fd, request, 0, nullptr), -EBUSY);
    EXPECT_EQ(drmModeAtomicCommit(bogus_fd, request, DRM_MODE_ATOMIC_TEST_ONLY, nullptr), 0);
    EXPECT_EQ(mock.atomic_visible_properties().at({plane_id, framebuffer_property_id}), initial_framebuffer_id);
    drmModeAtomicFree(request);

    EXPECT_THAT(mock.atomic_commits(), ElementsAre(
        AtomicCommit{first_request_id, bogus_fd, DRM_MODE_ATOMIC_TEST_ONLY, -EIO, {framebuffer_property}},
        AtomicCommit{first_request_id, bogus_fd, 0, 0, {framebuffer_property}},
        AtomicCommit{first_request_id, bogus_fd, 0, -EBUSY, {framebuffer_property, replacement_framebuffer}},
        AtomicCommit{first_request_id, bogus_fd, DRM_MODE_ATOMIC_TEST_ONLY, 0,
                     {framebuffer_property, replacement_framebuffer}}));
}

TEST_F(MockDRMAtomicRecordingTest, records_failed_allocation_add_and_free_in_order_under_overrides)
{
    int const overridden_add_result{17};
    EXPECT_CALL(mock, drmModeAtomicAlloc()).WillOnce(Return(nullptr)).WillOnce(DoDefault());
    EXPECT_EQ(drmModeAtomicAlloc(), nullptr);
    auto const request = drmModeAtomicAlloc();
    ASSERT_NE(request, nullptr);
    EXPECT_CALL(mock, drmModeAtomicAddProperty(request, _, _, _))
        .WillOnce(Return(-ENOSPC))
        .WillOnce(Return(overridden_add_result))
        .WillRepeatedly(DoDefault());
    EXPECT_EQ(drmModeAtomicAddProperty(request, plane_id, framebuffer_property_id, initial_framebuffer_id), -ENOSPC);
    EXPECT_EQ(drmModeAtomicAddProperty(request, connector_id, connector_crtc_property_id, assigned_crtc_id),
              overridden_add_result);
    EXPECT_EQ(drmModeAtomicAddProperty(request, crtc_id, mode_property_id, mode_blob_id), 2);
    EXPECT_CALL(mock, drmModeAtomicFree(request));
    drmModeAtomicFree(request);

    EXPECT_THAT(mock.atomic_requests(), ElementsAre(
        mtd::FreedAtomicRequest(first_request_id, ElementsAre(connector_assignment, mode_property))));
    EXPECT_THAT(mock.atomic_operations(), ElementsAre(
        AtomicOperation{AtomicOperationKind::allocate, 0, -1},
        AtomicOperation{AtomicOperationKind::allocate, first_request_id, 0},
        AtomicOperation{AtomicOperationKind::add_property, first_request_id, -ENOSPC},
        AtomicOperation{AtomicOperationKind::add_property, first_request_id, overridden_add_result},
        AtomicOperation{AtomicOperationKind::add_property, first_request_id, 2},
        AtomicOperation{AtomicOperationKind::free, first_request_id, 0}));
    EXPECT_TRUE(mock.atomic_commits().empty());
    EXPECT_TRUE(mock.atomic_visible_properties().empty());
}

TEST_F(MockDRMAtomicRecordingTest, tracks_distinct_lifetimes_when_overridden_allocation_reuses_handle)
{
    int const overridden_add_result{9};
    int storage{};
    auto const handle = reinterpret_cast<drmModeAtomicReqPtr>(&storage);
    EXPECT_CALL(mock, drmModeAtomicAlloc()).WillOnce(Return(handle)).WillOnce(Return(handle));
    EXPECT_CALL(mock, drmModeAtomicFree(handle)).Times(2);
    EXPECT_EQ(drmModeAtomicAlloc(), handle);
    EXPECT_EQ(drmModeAtomicAddProperty(handle, plane_id, framebuffer_property_id, initial_framebuffer_id), -EINVAL);
    drmModeAtomicFree(handle);
    EXPECT_EQ(drmModeAtomicAlloc(), handle);
    EXPECT_CALL(mock, drmModeAtomicAddProperty(handle, connector_id, connector_crtc_property_id, assigned_crtc_id))
        .WillOnce(Return(overridden_add_result));
    EXPECT_EQ(drmModeAtomicAddProperty(handle, connector_id, connector_crtc_property_id, assigned_crtc_id),
              overridden_add_result);
    EXPECT_CALL(mock, drmModeAtomicCommit(bogus_fd, handle, 0, nullptr)).WillOnce(Return(0));
    EXPECT_EQ(drmModeAtomicCommit(bogus_fd, handle, 0, nullptr), 0);
    drmModeAtomicFree(handle);
    EXPECT_THAT(mock.atomic_requests(), ElementsAre(
        mtd::FreedAtomicRequest(first_request_id),
        mtd::FreedAtomicRequest(second_request_id)));
    EXPECT_THAT(mock.atomic_commits(), ElementsAre(
        AtomicCommit{second_request_id, bogus_fd, 0, 0, {connector_assignment}}));
    EXPECT_EQ(mock.atomic_visible_properties().at({connector_id, connector_crtc_property_id}), assigned_crtc_id);
    EXPECT_THAT(mock.atomic_operations(), ElementsAre(
        AtomicOperation{AtomicOperationKind::allocate, first_request_id, 0},
        AtomicOperation{AtomicOperationKind::add_property, first_request_id, -EINVAL},
        AtomicOperation{AtomicOperationKind::free, first_request_id, 0},
        AtomicOperation{AtomicOperationKind::allocate, second_request_id, 0},
        AtomicOperation{AtomicOperationKind::add_property, second_request_id, overridden_add_result},
        AtomicOperation{AtomicOperationKind::commit, second_request_id, 0},
        AtomicOperation{AtomicOperationKind::free, second_request_id, 0}));
}

TEST(MockDRMAtomicTest, reports_unfreed_overridden_allocation)
{
    EXPECT_NONFATAL_FAILURE(
        {
            NiceMock<mtd::MockDRM> mock;
            int storage{};
            auto const handle = reinterpret_cast<drmModeAtomicReqPtr>(&storage);
            EXPECT_CALL(mock, drmModeAtomicAlloc()).WillOnce(Return(handle));
            EXPECT_EQ(drmModeAtomicAlloc(), handle);
        },
        "Atomic DRM requests were not freed");
}

TEST_F(MockDRMAtomicRecordingTest, invalid_commits_still_reach_gmock_and_cannot_change_visible_state)
{
    EXPECT_CALL(mock, drmModeAtomicCommit(bogus_fd, nullptr, 0, nullptr)).WillOnce(Return(0));
    EXPECT_EQ(drmModeAtomicCommit(bogus_fd, nullptr, 0, nullptr), 0);
    drmModeAtomicFree(nullptr);
    EXPECT_THAT(mock.atomic_commits(), ElementsAre(AtomicCommit{0, bogus_fd, 0, 0, {}}));
    EXPECT_THAT(mock.atomic_operations(), ElementsAre(
        AtomicOperation{AtomicOperationKind::commit, 0, 0},
        AtomicOperation{AtomicOperationKind::free, 0, 0}));
    EXPECT_TRUE(mock.atomic_visible_properties().empty());
}

TEST_F(MockDRMAtomicRecordingTest, default_add_and_commit_reject_invalid_requests_before_and_after_free)
{
    int unknown_request_storage{};
    auto const unknown_request = reinterpret_cast<drmModeAtomicReqPtr>(&unknown_request_storage);
    EXPECT_EQ(drmModeAtomicAddProperty(nullptr, plane_id, framebuffer_property_id, initial_framebuffer_id), -EINVAL);
    EXPECT_EQ(drmModeAtomicAddProperty(unknown_request, plane_id, framebuffer_property_id, initial_framebuffer_id),
              -EINVAL);
    EXPECT_EQ(drmModeAtomicCommit(bogus_fd, nullptr, 0, nullptr), -EINVAL);
    EXPECT_EQ(drmModeAtomicCommit(bogus_fd, unknown_request, 0, nullptr), -EINVAL);

    auto const request = drmModeAtomicAlloc();
    ASSERT_NE(request, nullptr);
    EXPECT_EQ(drmModeAtomicCommit(bogus_fd, request, 0, nullptr), 0);
    EXPECT_EQ(drmModeAtomicAddProperty(request, 0, framebuffer_property_id, initial_framebuffer_id), -EINVAL);
    EXPECT_EQ(drmModeAtomicAddProperty(request, plane_id, 0, initial_framebuffer_id), -EINVAL);
    EXPECT_EQ(drmModeAtomicAddProperty(request, plane_id, framebuffer_property_id, initial_framebuffer_id), 1);
    EXPECT_EQ(drmModeAtomicAddProperty(request, connector_id, connector_crtc_property_id, assigned_crtc_id), 2);
    EXPECT_THAT(mock.atomic_requests()[0].properties, ElementsAre(
        framebuffer_property, connector_assignment));
    EXPECT_THAT(mock.atomic_operations(), ElementsAre(
        AtomicOperation{AtomicOperationKind::add_property, 0, -EINVAL},
        AtomicOperation{AtomicOperationKind::add_property, 0, -EINVAL},
        AtomicOperation{AtomicOperationKind::commit, 0, -EINVAL},
        AtomicOperation{AtomicOperationKind::commit, 0, -EINVAL},
        AtomicOperation{AtomicOperationKind::allocate, first_request_id, 0},
        AtomicOperation{AtomicOperationKind::commit, first_request_id, 0},
        AtomicOperation{AtomicOperationKind::add_property, first_request_id, -EINVAL},
        AtomicOperation{AtomicOperationKind::add_property, first_request_id, -EINVAL},
        AtomicOperation{AtomicOperationKind::add_property, first_request_id, 1},
        AtomicOperation{AtomicOperationKind::add_property, first_request_id, 2}));
    drmModeAtomicFree(request);
    EXPECT_EQ(drmModeAtomicAddProperty(request, plane_id, framebuffer_property_id, initial_framebuffer_id), -EINVAL);
    EXPECT_EQ(drmModeAtomicCommit(bogus_fd, request, 0, nullptr), -EINVAL);
    EXPECT_THAT(mock.atomic_commits(), ElementsAre(
        AtomicCommit{0, bogus_fd, 0, -EINVAL, {}},
        AtomicCommit{0, bogus_fd, 0, -EINVAL, {}},
        AtomicCommit{first_request_id, bogus_fd, 0, 0, {}},
        AtomicCommit{0, bogus_fd, 0, -EINVAL, {}}));
}

TEST_F(AtomicUpdateTest, allocation_failure_does_not_create_request_or_cleanup)
{
    EXPECT_CALL(mock, drmModeAtomicAlloc()).WillOnce(Return(nullptr));
    EXPECT_CALL(mock, drmModeAtomicAddProperty(_, _, _, _)).Times(0);
    EXPECT_CALL(mock, drmModeAtomicCommit(_, _, _, _)).Times(0);
    EXPECT_CALL(mock, drmModeAtomicFree(_)).Times(0);

    EXPECT_THAT([]
    {
        mga::AtomicUpdate update;
    }, ThrowsMessage<mga::AtomicUpdateError>(HasSubstr("Failed to allocate Atomic DRM update request")));
}

TEST_F(AtomicUpdateTest, commit_failure_frees_request_and_does_not_change_visible_state)
{
    auto const properties = framebuffer_properties();
    AtomicProperty const expected_property{plane_id, framebuffer_property_id, framebuffer_id};

    {
        mga::AtomicUpdate update;
        auto const request = mock.atomic_requests().back().handle;
        Sequence sequence;
        EXPECT_CALL(mock, drmModeAtomicAddProperty(request, plane_id, framebuffer_property_id, framebuffer_id))
            .InSequence(sequence);
        EXPECT_CALL(mock, drmModeAtomicCommit(bogus_fd, request, commit_flags, nullptr))
            .InSequence(sequence).WillOnce(Return(-EIO));
        EXPECT_CALL(mock, drmModeAtomicFree(request)).InSequence(sequence);
        update.add_property(properties, framebuffer_property_name, framebuffer_id);
        EXPECT_EQ(update.commit(bogus_fd, commit_flags), -EIO);
        EXPECT_TRUE(mock.atomic_visible_properties().empty());
    }

    EXPECT_THAT(mock.atomic_requests(), ElementsAre(
        mtd::FreedAtomicRequest(first_request_id, ElementsAre(expected_property))));
    EXPECT_THAT(mock.atomic_commits(), ElementsAre(
        mtd::AtomicCommitWith(first_request_id, bogus_fd, commit_flags, -EIO, ElementsAre(expected_property))));
    EXPECT_THAT(mock.atomic_operations(), ElementsAre(
        AtomicOperation{AtomicOperationKind::allocate, first_request_id, 0},
        AtomicOperation{AtomicOperationKind::add_property, first_request_id, 1},
        AtomicOperation{AtomicOperationKind::commit, first_request_id, -EIO},
        AtomicOperation{AtomicOperationKind::free, first_request_id, 0}));
}
