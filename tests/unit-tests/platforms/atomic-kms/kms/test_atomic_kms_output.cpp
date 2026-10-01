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

#include "atomic_kms_output.h"

#include <mir/graphics/kms/drm_mode_resources.h>
#include <mir/graphics/kms_framebuffer.h>
#include <mir/graphics/gamma_curves.h>
#include <mir/test/doubles/mock_drm.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cerrno>
#include <memory>
#include <span>
#include <vector>

namespace mga = mir::graphics::atomic;
namespace mg = mir::graphics;
namespace mgk = mir::graphics::kms;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
constexpr int mode_width{1920};
constexpr int mode_height{1080};
geom::Size const requested_mode_size{mode_width, mode_height};
geom::Size const smaller_crtc_size{1280, 720};
geom::Displacement const source_offset{17, 29};
mg::GammaCurves const sample_gamma{{1}, {2}, {3}};

auto kms_fixed_point(uint32_t value) -> uint64_t
{
    constexpr int fractional_bits{16};
    return static_cast<uint64_t>(value) << fractional_bits;
}

class StubFramebuffer : public mg::FBHandle
{
public:
    explicit StubFramebuffer(uint32_t id, geom::Size size = requested_mode_size) : id_{id}, size_{size} {}

    operator uint32_t() const override
    {
        return id_;
    }

    auto size() const -> geom::Size override
    {
        return size_;
    }

private:
    uint32_t const id_;
    geom::Size const size_;
};

using AtomicProperty = mtd::MockDRM::AtomicProperty;
using AtomicOperation = mtd::MockDRM::AtomicOperation;
using AtomicOperationKind = mtd::MockDRM::AtomicOperationKind;

struct CommitExpectation
{
    size_t request_index{0};
    size_t preceding_operations{0};
    int result{0};
};

auto expected_add_operations(size_t request_id, size_t property_count) -> std::vector<AtomicOperation>
{
    std::vector<AtomicOperation> operations{{AtomicOperationKind::allocate, request_id, 0}};
    for (size_t index = 0; index < property_count; ++index)
        operations.push_back({AtomicOperationKind::add_property, request_id, static_cast<int>(index + 1)});
    return operations;
}

void expect_commit_request(
    mtd::MockDRM const& mock, uint32_t flags, std::vector<AtomicProperty> const& properties,
    CommitExpectation const expected = {})
{
    ASSERT_EQ(mock.atomic_requests().size(), expected.request_index + 1);
    auto const& request = mock.atomic_requests()[expected.request_index];
    EXPECT_THAT(request, mtd::FreedAtomicRequest(request.id, ElementsAreArray(properties)));
    EXPECT_THAT(mock.atomic_commits(), ElementsAre(mtd::AtomicCommitWith(
        request.id, mtd::IsFdOfDevice("/dev/dri/card0"), flags, expected.result, ElementsAreArray(properties))));

    auto expected_operations = expected_add_operations(request.id, properties.size());
    expected_operations.push_back({AtomicOperationKind::commit, request.id, expected.result});
    expected_operations.push_back({AtomicOperationKind::free, request.id, 0});
    ASSERT_EQ(mock.atomic_operations().size(), expected.preceding_operations + expected_operations.size());
    EXPECT_THAT(std::span{mock.atomic_operations()}.subspan(expected.preceding_operations),
                ElementsAreArray(expected_operations));
}

void expect_failed_request(
    mtd::MockDRM const& mock, std::vector<AtomicProperty> const& successful_properties)
{
    ASSERT_EQ(mock.atomic_requests().size(), 1u);
    auto const& request = mock.atomic_requests().front();
    EXPECT_THAT(request, mtd::FreedAtomicRequest(request.id, ElementsAreArray(successful_properties)));
    EXPECT_TRUE(mock.atomic_commits().empty());
    EXPECT_TRUE(mock.atomic_visible_properties().empty());

    auto expected_operations = expected_add_operations(request.id, successful_properties.size());
    expected_operations.push_back({AtomicOperationKind::add_property, request.id, -EIO});
    expected_operations.push_back({AtomicOperationKind::free, request.id, 0});
    EXPECT_THAT(mock.atomic_operations(), ElementsAreArray(expected_operations));
}

void expect_failed_commit(
    mtd::MockDRM const& mock, uint32_t flags, std::vector<AtomicProperty> const& properties)
{
    expect_commit_request(mock, flags, properties, {.result = -EIO});
    EXPECT_TRUE(mock.atomic_visible_properties().empty());
}

class AtomicKMSOutputTest : public Test
{
public:
    static constexpr uint32_t connector_crtc_property_id{101};
    static constexpr uint32_t crtc_active_property_id{201};
    static constexpr uint32_t crtc_mode_property_id{202};
    static constexpr uint32_t crtc_gamma_lut_property_id{203};
    static constexpr uint32_t plane_type_property_id{301};
    static constexpr uint32_t plane_src_x_property_id{302};
    static constexpr uint32_t plane_src_y_property_id{303};
    static constexpr uint32_t plane_src_w_property_id{304};
    static constexpr uint32_t plane_src_h_property_id{305};
    static constexpr uint32_t plane_crtc_x_property_id{306};
    static constexpr uint32_t plane_crtc_y_property_id{307};
    static constexpr uint32_t plane_crtc_w_property_id{308};
    static constexpr uint32_t plane_crtc_h_property_id{309};
    static constexpr uint32_t plane_crtc_property_id{310};
    static constexpr uint32_t plane_fb_property_id{311};
    static constexpr uint32_t mode_blob_id{1};
    static constexpr uint32_t gamma_lut_blob_id{2};
    static constexpr uint32_t primary_framebuffer_id{77};
    static constexpr uint32_t flip_framebuffer_id{88};
    static constexpr uint32_t connector_id{30};
    static constexpr uint32_t encoder_id{20};
    static constexpr uint32_t crtc_id{10};
    static constexpr uint32_t plane_id{40};
    static constexpr uint32_t primary_crtc_mask{1};

    AtomicKMSOutputTest()
        : drm_fd{mock_drm.open("/dev/dri/card0", 0)}
    {
        setup_drm_objects();

        ON_CALL(mock_drm, drmModeGetResources(_))
            .WillByDefault(Return(resources.resources_ptr()));
        ON_CALL(mock_drm, drmModeGetPlaneResources(_))
            .WillByDefault(Return(resources.plane_resources_ptr()));
        ON_CALL(mock_drm, drmModeGetConnector(_, connector_id))
            .WillByDefault(Return(&connector()));
        ON_CALL(mock_drm, drmModeGetEncoder(_, encoder_id))
            .WillByDefault(Return(resources.find_encoder(encoder_id)));
        ON_CALL(mock_drm, drmModeGetCrtc(_, crtc_id))
            .WillByDefault(Return(&crtc()));
        ON_CALL(mock_drm, drmModeGetPlane(_, plane_id))
            .WillByDefault(Return(resources.find_plane(plane_id)));
        ON_CALL(mock_drm, drmModeObjectGetProperties(_, _, _))
            .WillByDefault(
                [this](int, uint32_t id, uint32_t type)
                {
                    return resources.find_object_properties(id, type);
                });
        ON_CALL(mock_drm, drmModeGetProperty(_, _))
            .WillByDefault(
                [this](int, uint32_t property_id)
                {
                    return resources.find_property(property_id);
                });
    }

    auto output(geom::Displacement offset = {}) -> std::unique_ptr<mga::AtomicKMSOutput>
    {
        // Only the outer copy is owned here; its mode/encoder arrays belong to the fixture.
        mgk::DRMModeConnectorUPtr connector_copy{
            nullptr, std::default_delete<drmModeConnector>{}};
        connector_copy.reset(new drmModeConnector{connector()});
        auto result = std::make_unique<mga::AtomicKMSOutput>(
            std::move(drm_fd), std::move(connector_copy));
        result->configure(offset, 0);
        return result;
    }

    auto disabled_properties() const -> std::vector<AtomicProperty>
    {
        return {
            {connector_id, connector_crtc_property_id, 0},
            {crtc_id, crtc_active_property_id, 0},
            {crtc_id, crtc_mode_property_id, 0},
            {plane_id, plane_fb_property_id, 0},
            {plane_id, plane_crtc_property_id, 0}};
    }

    auto primary_request_properties(
        uint32_t framebuffer_id, uint64_t source_x, uint64_t source_y, bool activate_crtc,
        geom::Size size = requested_mode_size) const
        -> std::vector<AtomicProperty>
    {
        auto const width = size.width.as_uint32_t();
        auto const height = size.height.as_uint32_t();
        std::vector<AtomicProperty> properties{
            {crtc_id, crtc_mode_property_id, mode_blob_id},
            {connector_id, connector_crtc_property_id, crtc_id}};
        if (activate_crtc)
            properties.push_back({crtc_id, crtc_active_property_id, 1});
        properties.insert(properties.end(), {
            {plane_id, plane_src_x_property_id, source_x},
            {plane_id, plane_src_y_property_id, source_y},
            {plane_id, plane_src_w_property_id, kms_fixed_point(width)},
            {plane_id, plane_src_h_property_id, kms_fixed_point(height)},
            {plane_id, plane_crtc_x_property_id, 0},
            {plane_id, plane_crtc_y_property_id, 0},
            {plane_id, plane_crtc_w_property_id, width},
            {plane_id, plane_crtc_h_property_id, height},
            {plane_id, plane_crtc_property_id, crtc_id},
            {plane_id, plane_fb_property_id, framebuffer_id}});
        return properties;
    }

    void expect_gamma_blob_lifetime(uint32_t blob_id, Sequence const& destruction_order)
    {
        EXPECT_CALL(mock_drm, drmModeCreatePropertyBlob(_, _, _, _))
            .WillOnce(DoAll(SetArgPointee<3>(blob_id), Return(0)));
        EXPECT_CALL(mock_drm, drmModeDestroyPropertyBlob(_, blob_id)).InSequence(destruction_order);
    }

    auto crtc() -> drmModeCrtc&
    {
        return *resources.find_crtc(crtc_id);
    }

    auto connector() -> drmModeConnector&
    {
        return *resources.find_connector(connector_id);
    }

    void expect_crtc_reusable_after_failure(mga::AtomicKMSOutput& output_under_test)
    {
        auto const preceding_operations = mock_drm.atomic_operations().size();
        output_under_test.set_power_mode(mir_power_mode_off);
        expect_commit_request(mock_drm, DRM_MODE_ATOMIC_ALLOW_MODESET,
                              {{crtc_id, crtc_active_property_id, 0}},
                              {.request_index = 1, .preceding_operations = preceding_operations});
    }

    NiceMock<mtd::MockDRM> mock_drm;
    mir::Fd drm_fd;

private:
    void setup_drm_objects()
    {
        constexpr uint16_t horizontal_total{2000};
        constexpr uint16_t vertical_total{1100};
        constexpr uint32_t pixel_clock_khz{148500};
        drmModeModeInfo mode{};
        mode.hdisplay = mode_width;
        mode.vdisplay = mode_height;
        mode.htotal = horizontal_total;
        mode.vtotal = vertical_total;
        mode.clock = pixel_clock_khz;
        modes.push_back(mode);

        resources.reset();
        resources.add_crtc(crtc_id, mode);
        crtc().width = mode_width;
        crtc().height = mode_height;
        resources.add_encoder(encoder_id, crtc_id, primary_crtc_mask);
        resources.add_connector(
            connector_id, DRM_MODE_CONNECTOR_VGA, DRM_MODE_CONNECTED, encoder_id, modes, encoder_ids, {});
        connector().connector_type_id = 1;
        resources.add_plane(plane_id, primary_crtc_mask);
        resources.prepare();

        resources.add_property(connector_id, DRM_MODE_OBJECT_CONNECTOR, connector_crtc_property_id, "CRTC_ID");
        resources.add_property(crtc_id, DRM_MODE_OBJECT_CRTC, crtc_active_property_id, "ACTIVE");
        resources.add_property(crtc_id, DRM_MODE_OBJECT_CRTC, crtc_mode_property_id, "MODE_ID");
        resources.add_property(crtc_id, DRM_MODE_OBJECT_CRTC, crtc_gamma_lut_property_id, "GAMMA_LUT");
        add_plane_property(plane_type_property_id, "type", DRM_PLANE_TYPE_PRIMARY);
        add_plane_property(plane_src_x_property_id, "SRC_X");
        add_plane_property(plane_src_y_property_id, "SRC_Y");
        add_plane_property(plane_src_w_property_id, "SRC_W");
        add_plane_property(plane_src_h_property_id, "SRC_H");
        add_plane_property(plane_crtc_x_property_id, "CRTC_X");
        add_plane_property(plane_crtc_y_property_id, "CRTC_Y");
        add_plane_property(plane_crtc_w_property_id, "CRTC_W");
        add_plane_property(plane_crtc_h_property_id, "CRTC_H");
        add_plane_property(plane_crtc_property_id, "CRTC_ID");
        add_plane_property(plane_fb_property_id, "FB_ID");
    }

    void add_plane_property(uint32_t id, char const* name, uint64_t value = 0)
    {
        resources.add_property(plane_id, DRM_MODE_OBJECT_PLANE, id, name, value);
    }

    mtd::FakeDRMResources resources;
    std::vector<drmModeModeInfo> modes;
    std::vector<uint32_t> encoder_ids{encoder_id};
};

struct FramebufferMismatch
{
    char const* name;
    geom::Size size;
};

class AtomicKMSFramebufferSizeTest :
    public AtomicKMSOutputTest, public WithParamInterface<FramebufferMismatch>
{
};

struct DisableCase
{
    char const* name;
    void (mga::AtomicKMSOutput::*operation)();
    bool disconnect_connector{false};
};

class AtomicKMSDisableTest : public AtomicKMSOutputTest, public WithParamInterface<DisableCase>
{
};

struct PrimaryPropertyFailureCase
{
    char const* name;
    bool activate_crtc;
    bool fail_last_property;
};

class PrimaryPropertyFailureTest :
    public AtomicKMSOutputTest,
    public WithParamInterface<PrimaryPropertyFailureCase>
{
};
}

TEST_F(AtomicKMSOutputTest, set_crtc_preserves_primary_request_order_and_modeset_flag)
{
    auto output_under_test = output(source_offset);
    auto const properties = primary_request_properties(
        primary_framebuffer_id, kms_fixed_point(source_offset.dx.as_uint32_t()),
        kms_fixed_point(source_offset.dy.as_uint32_t()), true);

    StubFramebuffer framebuffer{primary_framebuffer_id};
    ASSERT_TRUE(output_under_test->set_crtc(framebuffer));
    expect_commit_request(mock_drm, DRM_MODE_ATOMIC_ALLOW_MODESET, properties);
}

TEST_F(AtomicKMSOutputTest, page_flip_preserves_primary_request_and_uses_normal_commit)
{
    auto output_under_test = output(source_offset);
    auto const properties = primary_request_properties(
        flip_framebuffer_id, kms_fixed_point(source_offset.dx.as_uint32_t()),
        kms_fixed_point(source_offset.dy.as_uint32_t()), false);

    StubFramebuffer framebuffer{flip_framebuffer_id};
    ASSERT_TRUE(output_under_test->page_flip(framebuffer));
    expect_commit_request(mock_drm, 0, properties);
}

TEST_F(AtomicKMSOutputTest, set_crtc_uses_requested_mode_size_instead_of_current_crtc_size)
{
    crtc().width = smaller_crtc_size.width.as_uint32_t();
    crtc().height = smaller_crtc_size.height.as_uint32_t();
    crtc().mode.hdisplay = smaller_crtc_size.width.as_uint32_t();
    crtc().mode.vdisplay = smaller_crtc_size.height.as_uint32_t();
    auto output_under_test = output();
    StubFramebuffer framebuffer{primary_framebuffer_id, requested_mode_size};

    EXPECT_TRUE(output_under_test->set_crtc(framebuffer));
    expect_commit_request(mock_drm, DRM_MODE_ATOMIC_ALLOW_MODESET,
                          primary_request_properties(primary_framebuffer_id, 0, 0, true));
}

TEST_F(AtomicKMSOutputTest, page_flip_uses_current_crtc_size_instead_of_requested_mode_size)
{
    crtc().width = smaller_crtc_size.width.as_uint32_t();
    crtc().height = smaller_crtc_size.height.as_uint32_t();
    crtc().mode.hdisplay = smaller_crtc_size.width.as_uint32_t();
    crtc().mode.vdisplay = smaller_crtc_size.height.as_uint32_t();
    auto output_under_test = output();
    StubFramebuffer framebuffer{flip_framebuffer_id, smaller_crtc_size};

    EXPECT_TRUE(output_under_test->page_flip(framebuffer));
    expect_commit_request(mock_drm, 0,
                          primary_request_properties(flip_framebuffer_id, 0, 0, false, smaller_crtc_size));
}

TEST_P(AtomicKMSFramebufferSizeTest, page_flip_rejects_mismatch_without_allocating_request)
{
    auto output_under_test = output();
    StubFramebuffer framebuffer{flip_framebuffer_id, GetParam().size};

    EXPECT_FALSE(output_under_test->page_flip(framebuffer));
    EXPECT_TRUE(mock_drm.atomic_operations().empty());
}

INSTANTIATE_TEST_SUITE_P(
    FramebufferDimensions, AtomicKMSFramebufferSizeTest,
    Values(
        FramebufferMismatch{"WidthMismatch", {smaller_crtc_size.width, requested_mode_size.height}},
        FramebufferMismatch{"HeightMismatch", {requested_mode_size.width, smaller_crtc_size.height}}),
    [](TestParamInfo<FramebufferMismatch> const& info) { return info.param.name; });

TEST_F(AtomicKMSOutputTest, power_mode_submits_active_property)
{
    auto output_under_test = output();

    output_under_test->set_power_mode(mir_power_mode_off);
    expect_commit_request(mock_drm, DRM_MODE_ATOMIC_ALLOW_MODESET, {{crtc_id, crtc_active_property_id, 0}});
}

TEST_F(AtomicKMSOutputTest, gamma_submits_lut_property)
{
    uint16_t const red_sample{1};
    uint16_t const green_sample{2};
    uint16_t const blue_sample{3};
    auto output_under_test = output();
    Sequence destruction_order;
    expect_gamma_blob_lifetime(gamma_lut_blob_id, destruction_order);
    // configure() owns MODE_ID until output destruction; the temporary LUT is freed first.
    EXPECT_CALL(mock_drm, drmModeDestroyPropertyBlob(_, mode_blob_id)).InSequence(destruction_order);

    output_under_test->set_gamma({{red_sample}, {green_sample}, {blue_sample}});
    expect_commit_request(mock_drm, DRM_MODE_ATOMIC_ALLOW_MODESET,
                          {{crtc_id, crtc_gamma_lut_property_id, gamma_lut_blob_id}});

    output_under_test.reset();
}

TEST_P(AtomicKMSDisableTest, submits_all_disable_properties)
{
    auto output_under_test = output();

    if (GetParam().disconnect_connector)
        connector().connection = DRM_MODE_DISCONNECTED;
    ((*output_under_test).*GetParam().operation)();
    expect_commit_request(mock_drm, DRM_MODE_ATOMIC_ALLOW_MODESET, disabled_properties());
}

INSTANTIATE_TEST_SUITE_P(
    DisablePaths, AtomicKMSDisableTest,
    Values(
        DisableCase{"ClearCrtc", &mga::AtomicKMSOutput::clear_crtc},
        DisableCase{"ResetDisconnectedConnector", &mga::AtomicKMSOutput::reset, true},
        DisableCase{"RefreshDisconnectedConnector", &mga::AtomicKMSOutput::refresh_hardware_state, true}),
    [](TestParamInfo<DisableCase> const& info) { return info.param.name; });

TEST_F(AtomicKMSOutputTest, property_add_failure_prevents_commit)
{
    auto output_under_test = output();
    ON_CALL(mock_drm, drmModeAtomicAddProperty(_, crtc_id, crtc_active_property_id, 0))
        .WillByDefault(Return(-EIO));

    EXPECT_NO_THROW(output_under_test->set_power_mode(mir_power_mode_off));
    expect_failed_request(mock_drm, {});
}

TEST_P(PrimaryPropertyFailureTest, property_add_failure_returns_false_without_commit_and_keeps_crtc_reusable)
{
    auto const failure = GetParam();
    auto const framebuffer_id = failure.activate_crtc ? primary_framebuffer_id : flip_framebuffer_id;
    auto output_under_test = output();
    auto successful_properties = primary_request_properties(framebuffer_id, 0, 0, failure.activate_crtc);
    auto const failed_property = failure.fail_last_property ?
        successful_properties.back() : successful_properties.front();
    if (failure.fail_last_property)
        successful_properties.pop_back();
    else
        successful_properties.clear();

    EXPECT_CALL(mock_drm, drmModeAtomicAddProperty(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(mock_drm, drmModeAtomicAddProperty(
        _, failed_property.object_id, failed_property.property_id, failed_property.value))
        .WillOnce(Return(-EIO));
    StubFramebuffer framebuffer{framebuffer_id};

    EXPECT_FALSE(failure.activate_crtc ?
        output_under_test->set_crtc(framebuffer) : output_under_test->page_flip(framebuffer));
    expect_failed_request(mock_drm, successful_properties);
    expect_crtc_reusable_after_failure(*output_under_test);
}

INSTANTIATE_TEST_SUITE_P(
    PrimaryRequestFailures,
    PrimaryPropertyFailureTest,
    Values(
        PrimaryPropertyFailureCase{"SetCrtcFailsAtModeProperty", true, false},
        PrimaryPropertyFailureCase{"SetCrtcFailsAtFramebufferProperty", true, true},
        PrimaryPropertyFailureCase{"PageFlipFailsAtModeProperty", false, false},
        PrimaryPropertyFailureCase{"PageFlipFailsAtFramebufferProperty", false, true}),
    [](TestParamInfo<PrimaryPropertyFailureCase> const& info) { return info.param.name; });

TEST_F(AtomicKMSOutputTest, set_crtc_commit_failure_frees_request_without_changing_visible_state)
{
    auto output_under_test = output();
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, DRM_MODE_ATOMIC_ALLOW_MODESET, nullptr))
        .WillOnce(Return(-EIO));

    StubFramebuffer framebuffer{primary_framebuffer_id};
    EXPECT_FALSE(output_under_test->set_crtc(framebuffer));
    expect_failed_commit(mock_drm, DRM_MODE_ATOMIC_ALLOW_MODESET,
                         primary_request_properties(primary_framebuffer_id, 0, 0, true));
}

TEST_F(AtomicKMSOutputTest, page_flip_commit_failure_frees_request_without_changing_visible_state)
{
    auto output_under_test = output();
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, 0, nullptr)).WillOnce(Return(-EIO));

    StubFramebuffer framebuffer{flip_framebuffer_id};
    EXPECT_FALSE(output_under_test->page_flip(framebuffer));
    expect_failed_commit(mock_drm, 0, primary_request_properties(flip_framebuffer_id, 0, 0, false));
}

TEST_P(AtomicKMSDisableTest, property_add_failure_is_non_fatal_and_does_not_commit)
{
    auto output_under_test = output();
    if (GetParam().disconnect_connector)
        connector().connection = DRM_MODE_DISCONNECTED;
    ON_CALL(mock_drm, drmModeAtomicAddProperty(_, connector_id, connector_crtc_property_id, 0))
        .WillByDefault(Return(-EIO));

    EXPECT_NO_THROW(((*output_under_test).*GetParam().operation)());
    expect_failed_request(mock_drm, {});
}

TEST_F(AtomicKMSOutputTest, gamma_property_add_failure_is_non_fatal)
{
    uint32_t const failed_gamma_lut_blob_id{900};
    auto output_under_test = output();
    Sequence destruction_order;
    expect_gamma_blob_lifetime(failed_gamma_lut_blob_id, destruction_order);
    EXPECT_CALL(mock_drm, drmModeDestroyPropertyBlob(_, mode_blob_id)).InSequence(destruction_order);
    ON_CALL(mock_drm, drmModeAtomicAddProperty(_, crtc_id, crtc_gamma_lut_property_id, failed_gamma_lut_blob_id))
        .WillByDefault(Return(-EIO));

    EXPECT_NO_THROW(output_under_test->set_gamma(sample_gamma));
    expect_failed_request(mock_drm, {});

    output_under_test.reset();
}

TEST_F(AtomicKMSOutputTest, gamma_allocation_failure_destroys_lut_blob)
{
    uint32_t const failed_gamma_lut_blob_id{900};
    auto output_under_test = output();
    Sequence destruction_order;
    expect_gamma_blob_lifetime(failed_gamma_lut_blob_id, destruction_order);
    EXPECT_CALL(mock_drm, drmModeDestroyPropertyBlob(_, mode_blob_id)).InSequence(destruction_order);
    EXPECT_CALL(mock_drm, drmModeAtomicAlloc()).WillOnce(Return(nullptr));

    EXPECT_NO_THROW(output_under_test->set_gamma(sample_gamma));
    EXPECT_TRUE(mock_drm.atomic_requests().empty());
    EXPECT_TRUE(mock_drm.atomic_commits().empty());
    EXPECT_THAT(mock_drm.atomic_operations(), ElementsAre(AtomicOperation{AtomicOperationKind::allocate, 0, -1}));

    output_under_test.reset();
}
