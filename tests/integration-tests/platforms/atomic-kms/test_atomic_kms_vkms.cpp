/*
 * Copyright © Canonical Ltd.
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
 */

#include "vkms_device.h"

#include "src/server/console/minimal_console_services.h"
#include "src/server/report/null_report_factory.h"

#include <mir/emergency_cleanup_registry.h>
#include <mir/glib_main_loop.h>
#include <mir/fd.h>
#include <mir/graphics/default_display_configuration_policy.h>
#include <mir/graphics/display.h>
#include <mir/graphics/display_configuration.h>
#include <mir/graphics/platform.h>
#include <mir/options/program_option.h>
#include <mir/shared_library.h>
#include <mir/udev/wrapper.h>
#include <mir/test/auto_unblock_thread.h>
#include <mir/test/doubles/stub_gl_config.h>
#include <mir/test/signal.h>
#include <mir/time/steady_clock.h>
#include <mir_test_framework/executable_path.h>

#include <boost/program_options/options_description.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <unistd.h>

namespace fs = std::filesystem;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;
namespace mtv = mir::test::vkms;

namespace
{
using namespace std::chrono_literals;
using testing::Eq;
using testing::IsEmpty;
using testing::Not;
using testing::SizeIs;

struct Scenario
{
    mtv::Topology initial;
    std::vector<mtv::ConnectorStatusChange> changes;
};

Scenario one_connected_pipeline()
{
    return {
        mtv::TopologyBuilder{}
            .add_crtc("crtc-a")
            .add_plane("primary-a", mtv::PlaneType::primary, {"crtc-a"})
            .add_encoder("encoder-a", {"crtc-a"})
            .add_connector("connector-a", mtv::ConnectorStatus::connected, {"encoder-a"})
            .build(),
        {}};
}

Scenario selected_target_scenario()
{
    return {
        mtv::TopologyBuilder{}
            .add_crtc("crtc-a")
            .add_crtc("crtc-b")
            .add_plane("primary-a", mtv::PlaneType::primary, {"crtc-a"})
            .add_plane("primary-b", mtv::PlaneType::primary, {"crtc-b"})
            .add_encoder("encoder-a", {"crtc-a"})
            .add_encoder("encoder-b", {"crtc-b"})
            .add_connector("connector-a", mtv::ConnectorStatus::connected, {"encoder-a"})
            .add_connector("connector-b", mtv::ConnectorStatus::disconnected, {"encoder-b"})
            .build(),
        {}};
}

Scenario crossed_routes_scenario()
{
    return {
        mtv::TopologyBuilder{}
            .add_crtc("crtc-a")
            .add_crtc("crtc-b")
            .add_plane("primary-a", mtv::PlaneType::primary, {"crtc-b"})
            .add_plane("primary-b", mtv::PlaneType::primary, {"crtc-a"})
            .add_encoder("encoder-a", {"crtc-b"})
            .add_encoder("encoder-b", {"crtc-a"})
            .add_connector("connector-a", mtv::ConnectorStatus::connected, {"encoder-a"})
            .add_connector("connector-b", mtv::ConnectorStatus::connected, {"encoder-b"})
            .build(),
        {}};
}

Scenario hotplug_scenario()
{
    return {
        mtv::TopologyBuilder{}
            .add_crtc("crtc-a")
            .add_crtc("crtc-b")
            .add_plane("primary-a", mtv::PlaneType::primary, {"crtc-a"})
            .add_plane("primary-b", mtv::PlaneType::primary, {"crtc-b"})
            .add_encoder("encoder-a", {"crtc-a"})
            .add_encoder("encoder-b", {"crtc-b"})
            .add_connector("connector-a", mtv::ConnectorStatus::connected, {"encoder-a"})
            .add_connector("connector-b", mtv::ConnectorStatus::connected, {"encoder-b"})
            .build(),
        {
            {"connector-b", mtv::ConnectorStatus::disconnected},
            {"connector-b", mtv::ConnectorStatus::connected},
        }};
}

class NoopEmergencyCleanup : public mir::EmergencyCleanupRegistry
{
public:
    void add(mir::EmergencyCleanupHandler const&) override
    {
    }

    void add(mir::ModuleEmergencyCleanupHandler) override
    {
    }
};

struct OutputSnapshot
{
    int id;
    std::string name;
    bool connected;
    bool used;
    std::vector<mg::DisplayConfigurationMode> modes;
    uint32_t preferred_mode_index;
    uint32_t current_mode_index;
    MirPowerMode power_mode;
    bool raw_edid_empty;
};

std::vector<OutputSnapshot> outputs_of(mg::Display& display)
{
    auto const configuration = display.configuration();
    std::vector<OutputSnapshot> outputs;
    configuration->for_each_output(
        [&](mg::DisplayConfigurationOutput const& output)
        {
            outputs.push_back(OutputSnapshot{
                output.id.as_value(),
                output.name,
                output.connected,
                output.used,
                output.modes,
                output.preferred_mode_index,
                output.current_mode_index,
                output.power_mode,
                output.display_info.raw_edid.empty()});
        });
    return outputs;
}

size_t sync_group_count(mg::Display& display)
{
    size_t count{0};
    display.for_each_display_sync_group([&](mg::DisplaySyncGroup&) { ++count; });
    return count;
}

std::set<std::pair<int, std::string>> output_id_names(std::vector<OutputSnapshot> const& outputs)
{
    std::set<std::pair<int, std::string>> result;
    for (auto const& output : outputs)
        result.emplace(output.id, output.name);
    return result;
}

uint64_t property_value(int fd, uint32_t object_id, uint32_t object_type, char const* property_name)
{
    std::unique_ptr<drmModeObjectProperties, decltype(&drmModeFreeObjectProperties)> const properties{
        drmModeObjectGetProperties(fd, object_id, object_type),
        &drmModeFreeObjectProperties};
    if (!properties)
        throw std::runtime_error{"drmModeObjectGetProperties failed"};

    for (uint32_t i = 0; i < properties->count_props; ++i)
    {
        std::unique_ptr<drmModePropertyRes, decltype(&drmModeFreeProperty)> const property{
            drmModeGetProperty(fd, properties->props[i]),
            &drmModeFreeProperty};
        if (property && std::strcmp(property->name, property_name) == 0)
            return properties->prop_values[i];
    }
    throw std::runtime_error{std::string{"DRM object has no property named "} + property_name};
}

struct KernelConnector
{
    uint32_t id;
    std::string name;
    drmModeConnection connection;
    uint32_t crtc_id;
};

struct KernelCrtc
{
    uint32_t id;
    bool active;
    uint32_t width;
    uint32_t height;
};

struct KernelState
{
    explicit KernelState(std::string const& devnode)
    {
        fd = mir::Fd{::open(devnode.c_str(), O_RDWR | O_CLOEXEC)};
        if (fd == mir::Fd::invalid)
            throw std::runtime_error{"failed to open " + devnode + " for independent libdrm assertions"};

        if (drmSetClientCap(fd, DRM_CLIENT_CAP_ATOMIC, 1) != 0)
            throw std::runtime_error{"failed to enable atomic libdrm properties for " + devnode};

        std::unique_ptr<drmModeRes, decltype(&drmModeFreeResources)> const resources{
            drmModeGetResources(fd),
            &drmModeFreeResources};
        if (!resources)
            throw std::runtime_error{"drmModeGetResources failed for " + devnode};

        for (int i = 0; i < resources->count_connectors; ++i)
        {
            std::unique_ptr<drmModeConnector, decltype(&drmModeFreeConnector)> const connector{
                drmModeGetConnector(fd, resources->connectors[i]),
                &drmModeFreeConnector};
            if (!connector)
                throw std::runtime_error{"drmModeGetConnector failed"};

            auto const* type_name = drmModeGetConnectorTypeName(connector->connector_type);
            connectors.push_back(KernelConnector{
                connector->connector_id,
                std::string{type_name ? type_name : "Unknown"} + "-" +
                    std::to_string(connector->connector_type_id),
                connector->connection,
                static_cast<uint32_t>(property_value(
                    fd,
                    connector->connector_id,
                    DRM_MODE_OBJECT_CONNECTOR,
                    "CRTC_ID"))});
        }

        for (int i = 0; i < resources->count_crtcs; ++i)
        {
            std::unique_ptr<drmModeCrtc, decltype(&drmModeFreeCrtc)> const crtc{
                drmModeGetCrtc(fd, resources->crtcs[i]),
                &drmModeFreeCrtc};
            if (!crtc)
                throw std::runtime_error{"drmModeGetCrtc failed"};

            crtcs.push_back(KernelCrtc{
                crtc->crtc_id,
                property_value(fd, crtc->crtc_id, DRM_MODE_OBJECT_CRTC, "ACTIVE") != 0,
                crtc->width,
                crtc->height});
        }
    }

    KernelState(KernelState const&) = delete;
    auto operator=(KernelState const&) -> KernelState& = delete;

    auto crtc(uint32_t id) const -> KernelCrtc const&
    {
        auto const found = std::ranges::find(crtcs, id, &KernelCrtc::id);
        if (found == crtcs.end())
            throw std::runtime_error{"connector refers to an unknown CRTC"};
        return *found;
    }
    mir::Fd fd;
    std::vector<KernelConnector> connectors;
    std::vector<KernelCrtc> crtcs;
};

class PlatformSession
{
public:
    PlatformSession(mtv::VkmsDevice& device, std::shared_ptr<mg::DisplayConfigurationPolicy> policy)
        : module{mir_test_framework::server_platform("graphics-atomic-kms")},
          options{std::make_shared<mir::options::ProgramOption>()},
          cleanup{std::make_shared<NoopEmergencyCleanup>()},
          console{std::make_shared<mir::MinimalConsoleServices>()},
          policy{std::move(policy)},
          main_loop{std::make_shared<mir::time::SteadyClock>()}
    {
        auto const add_options = module.load_function<mg::AddPlatformOptions>("add_graphics_platform_options");
        auto const create_platform = module.load_function<mg::CreateDisplayPlatform>("create_display_platform");

        boost::program_options::options_description option_description;
        add_options(option_description);
        char const* argv[]{"mir_atomic_kms_vkms_tests", nullptr};
        options->parse_arguments(option_description, 1, argv);

        mg::SupportedDevice selected_device{device.udev_device(), mg::probe::best, {}};
        platform = create_platform(
            selected_device,
            options,
            cleanup,
            console,
            mir::report::null_display_report());
        display = platform->create_display(this->policy, std::make_shared<mtd::StubGLConfig>());
        display->register_configuration_change_handler(
            main_loop,
            [this]
            {
                configuration_changed.raise();
            });
        loop_thread = mir::test::AutoUnblockThread{
            [this] { main_loop.stop(); },
            [this] { main_loop.run(); }};
    }

    ~PlatformSession()
    {
        if (display)
            main_loop.unregister_fd_handler(display.get());
        main_loop.stop();
        loop_thread.stop();
        display.reset();
        platform.reset();
    }

    bool apply_and_refresh(mtv::VkmsDevice& device, mtv::ConnectorStatusChange const& change)
    {
        configuration_changed.reset();
        device.apply(change);
        if (!configuration_changed.wait_for(2s))
            return false;

        auto configuration = display->configuration();
        policy->apply_to(*configuration);
        display->configure(*configuration);
        return true;
    }

    mir::SharedLibrary module;
    std::shared_ptr<mir::options::ProgramOption> options;
    std::shared_ptr<NoopEmergencyCleanup> cleanup;
    std::shared_ptr<mir::MinimalConsoleServices> console;
    std::shared_ptr<mg::DisplayConfigurationPolicy> policy;
    mir::UniqueModulePtr<mg::DisplayPlatform> platform;
    mir::UniqueModulePtr<mg::Display> display;
    mir::GLibMainLoop main_loop;
    mir::test::Signal configuration_changed;
    mir::test::AutoUnblockThread loop_thread;
};

class AtomicKmsVkmsTest : public testing::Test
{
public:
    auto make_device(mtv::Topology const& topology) -> mtv::VkmsDevice&
    {
        devices.push_back(std::make_unique<mtv::VkmsDevice>(topology));
        return *devices.back();
    }

    void start(mtv::VkmsDevice& device, std::shared_ptr<mg::DisplayConfigurationPolicy> policy)
    {
        session = std::make_unique<PlatformSession>(device, std::move(policy));
    }

    void TearDown() override
    {
        session.reset();

        std::vector<fs::path> paths;
        for (auto const& device : devices)
            paths.push_back(device->configfs_path());
        devices.clear();
        for (auto const& path : paths)
            EXPECT_FALSE(fs::exists(path)) << "VKMS configfs device was not removed: " << path;
    }

    std::vector<std::unique_ptr<mtv::VkmsDevice>> devices;
    std::unique_ptr<PlatformSession> session;
};

TEST_F(AtomicKmsVkmsTest, selected_vkms_device_is_exposed_through_platform_module)
{
    auto const decoy_scenario = one_connected_pipeline();
    auto const target_scenario = selected_target_scenario();
    make_device(decoy_scenario.initial);
    auto& target = make_device(target_scenario.initial);

    start(target, std::make_shared<mg::SingleDisplayConfigurationPolicy>());
    auto const outputs = outputs_of(*session->display);

    ASSERT_THAT(outputs, SizeIs(2));
    auto const connected = std::ranges::find(outputs, true, &OutputSnapshot::connected);
    auto const disconnected = std::ranges::find(outputs, false, &OutputSnapshot::connected);
    ASSERT_NE(connected, outputs.end());
    ASSERT_NE(disconnected, outputs.end());
    ASSERT_THAT(connected->modes, Not(IsEmpty()));
    ASSERT_LT(connected->preferred_mode_index, connected->modes.size());
    EXPECT_EQ(connected->modes[connected->preferred_mode_index].size, (geom::Size{1024, 768}));
    EXPECT_TRUE(connected->raw_edid_empty);
    EXPECT_THAT(disconnected->modes, IsEmpty());
    EXPECT_EQ(disconnected->current_mode_index, std::numeric_limits<uint32_t>::max());

    KernelState const kernel{target.devnode()};
    EXPECT_THAT(kernel.connectors, SizeIs(2));
    EXPECT_EQ(
        std::ranges::count(kernel.connectors, DRM_MODE_CONNECTED, &KernelConnector::connection),
        1);
    std::cout << "selected VKMS card exposed 2 target outputs; decoy had 1" << std::endl;
}

TEST_F(AtomicKmsVkmsTest, exclusive_crossed_routes_activate_two_outputs)
{
    auto const scenario = crossed_routes_scenario();
    auto& device = make_device(scenario.initial);
    start(device, std::make_shared<mg::SideBySideDisplayConfigurationPolicy>());

    auto const outputs = outputs_of(*session->display);
    ASSERT_THAT(outputs, SizeIs(2));
    EXPECT_THAT(outputs, testing::Each(testing::Field(&OutputSnapshot::connected, Eq(true))));
    EXPECT_THAT(outputs, testing::Each(testing::Field(&OutputSnapshot::used, Eq(true))));
    EXPECT_EQ(sync_group_count(*session->display), 2);

    KernelState const kernel{device.devnode()};
    ASSERT_THAT(kernel.connectors, SizeIs(2));
    ASSERT_THAT(kernel.crtcs, SizeIs(2));
    EXPECT_EQ(kernel.connectors[0].crtc_id, kernel.crtcs[1].id);
    EXPECT_EQ(kernel.connectors[1].crtc_id, kernel.crtcs[0].id);
    EXPECT_THAT(kernel.crtcs, testing::Each(testing::Field(&KernelCrtc::active, Eq(true))));
    std::cout << "exclusive routes attached connectors to crossed CRTCs; 2 CRTCs active" << std::endl;
}

TEST_F(AtomicKmsVkmsTest, connector_hotplug_updates_configuration_and_active_sinks)
{
    auto const scenario = hotplug_scenario();
    auto& device = make_device(scenario.initial);
    start(device, std::make_shared<mg::SideBySideDisplayConfigurationPolicy>());

    auto const before = outputs_of(*session->display);
    ASSERT_THAT(before, SizeIs(2));
    auto const identities = output_id_names(before);
    EXPECT_EQ(sync_group_count(*session->display), 2);

    ASSERT_TRUE(session->apply_and_refresh(device, scenario.changes[0]))
        << "no display-configuration callback after VKMS disconnect";
    auto const disconnected = outputs_of(*session->display);
    ASSERT_THAT(disconnected, SizeIs(2));
    EXPECT_EQ(output_id_names(disconnected), identities);
    EXPECT_EQ(std::ranges::count(disconnected, true, &OutputSnapshot::connected), 1);
    auto const offline = std::ranges::find(disconnected, false, &OutputSnapshot::connected);
    ASSERT_NE(offline, disconnected.end());
    EXPECT_THAT(offline->modes, IsEmpty());
    EXPECT_EQ(offline->current_mode_index, std::numeric_limits<uint32_t>::max());
    EXPECT_EQ(sync_group_count(*session->display), 1);
    EXPECT_EQ(
        std::ranges::count(
            KernelState{device.devnode()}.connectors,
            DRM_MODE_CONNECTED,
            &KernelConnector::connection),
        1);

    ASSERT_TRUE(session->apply_and_refresh(device, scenario.changes[1]))
        << "no display-configuration callback after VKMS reconnect";
    auto const reconnected = outputs_of(*session->display);
    ASSERT_THAT(reconnected, SizeIs(2));
    EXPECT_EQ(output_id_names(reconnected), identities);
    EXPECT_THAT(reconnected, testing::Each(testing::Field(&OutputSnapshot::connected, Eq(true))));
    EXPECT_THAT(reconnected, testing::Each(testing::Field(&OutputSnapshot::modes, Not(IsEmpty()))));
    EXPECT_EQ(sync_group_count(*session->display), 2);
    EXPECT_EQ(
        std::ranges::count(
            KernelState{device.devnode()}.connectors,
            DRM_MODE_CONNECTED,
            &KernelConnector::connection),
        2);
    std::cout << "hotplug callbacks refreshed stable outputs and sync groups 2 -> 1 -> 2" << std::endl;
}

TEST_F(AtomicKmsVkmsTest, configuring_output_changes_kernel_crtc_state)
{
    auto const scenario = one_connected_pipeline();
    auto& device = make_device(scenario.initial);
    start(device, std::make_shared<mg::SingleDisplayConfigurationPolicy>());

    auto const initial_outputs = outputs_of(*session->display);
    ASSERT_THAT(initial_outputs, SizeIs(1));
    auto const& output = initial_outputs.front();
    ASSERT_LT(output.preferred_mode_index, output.modes.size());
    auto const preferred_size = output.modes[output.preferred_mode_index].size;
    auto const alternate = std::ranges::find_if(
        output.modes,
        [&](mg::DisplayConfigurationMode const& mode) { return mode.size != preferred_size; });
    ASSERT_NE(alternate, output.modes.end())
        << "VKMS exposed no non-preferred mode with different dimensions";
    auto const alternate_index = static_cast<uint32_t>(std::distance(output.modes.begin(), alternate));
    auto const alternate_size = alternate->size;

    auto configure_output = [&](bool used, MirPowerMode power_mode, uint32_t mode_index)
    {
        auto configuration = session->display->configuration();
        configuration->for_each_output(
            [&](mg::UserDisplayConfigurationOutput& candidate)
            {
                if (candidate.id.as_value() == output.id)
                {
                    candidate.used = used;
                    candidate.power_mode = power_mode;
                    candidate.current_mode_index = mode_index;
                }
            });
        session->display->configure(*configuration);
    };

    configure_output(true, mir_power_mode_on, alternate_index);
    KernelState alternate_kernel{device.devnode()};
    ASSERT_THAT(alternate_kernel.connectors, SizeIs(1));
    ASSERT_NE(alternate_kernel.connectors[0].crtc_id, 0u);
    auto const active_crtc_id = alternate_kernel.connectors[0].crtc_id;
    auto const& alternate_crtc = alternate_kernel.crtc(active_crtc_id);
    EXPECT_TRUE(alternate_crtc.active);
    EXPECT_EQ(alternate_crtc.width, alternate_size.width.as_uint32_t());
    EXPECT_EQ(alternate_crtc.height, alternate_size.height.as_uint32_t());

    configure_output(false, mir_power_mode_off, alternate_index);
    KernelState unused_kernel{device.devnode()};
    ASSERT_THAT(unused_kernel.connectors, SizeIs(1));
    EXPECT_EQ(unused_kernel.connectors[0].crtc_id, 0u);
    EXPECT_FALSE(unused_kernel.crtc(active_crtc_id).active);

    configure_output(true, mir_power_mode_on, output.preferred_mode_index);
    KernelState preferred_kernel{device.devnode()};
    ASSERT_THAT(preferred_kernel.connectors, SizeIs(1));
    ASSERT_NE(preferred_kernel.connectors[0].crtc_id, 0u);
    auto const& preferred_crtc = preferred_kernel.crtc(preferred_kernel.connectors[0].crtc_id);
    EXPECT_TRUE(preferred_crtc.active);
    EXPECT_EQ(preferred_crtc.width, preferred_size.width.as_uint32_t());
    EXPECT_EQ(preferred_crtc.height, preferred_size.height.as_uint32_t());
    std::cout << "public configuration produced alternate active, detached inactive, preferred active kernel state"
              << std::endl;
}
}
