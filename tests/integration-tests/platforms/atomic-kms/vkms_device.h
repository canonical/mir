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

#ifndef MIR_TESTS_ATOMIC_KMS_VKMS_DEVICE_H_
#define MIR_TESTS_ATOMIC_KMS_VKMS_DEVICE_H_

#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <sys/types.h>

namespace mir::udev
{
class Device;
}

namespace mir::test::vkms
{
enum class PlaneType
{
    overlay = 0,
    primary = 1,
    cursor = 2,
};

enum class ConnectorStatus
{
    connected = 1,
    disconnected = 2,
    unknown = 3,
};

struct Crtc
{
    std::string name;
    bool writeback;
};

struct Plane
{
    std::string name;
    PlaneType type;
    std::vector<std::string> possible_crtcs;
};

struct Encoder
{
    std::string name;
    std::vector<std::string> possible_crtcs;
};

struct Connector
{
    std::string name;
    ConnectorStatus status;
    std::vector<std::string> possible_encoders;
};

struct Topology
{
    std::vector<Crtc> crtcs;
    std::vector<Plane> planes;
    std::vector<Encoder> encoders;
    std::vector<Connector> connectors;
};

struct ConnectorStatusChange
{
    std::string connector;
    ConnectorStatus status;
};

class TopologyBuilder
{
public:
    auto add_crtc(std::string name, bool writeback = false) -> TopologyBuilder&;
    auto add_plane(
        std::string name,
        PlaneType type,
        std::vector<std::string> possible_crtcs) -> TopologyBuilder&;
    auto add_encoder(std::string name, std::vector<std::string> possible_crtcs) -> TopologyBuilder&;
    auto add_connector(
        std::string name,
        ConnectorStatus status,
        std::vector<std::string> possible_encoders) -> TopologyBuilder&;
    auto build() const -> Topology;

private:
    Topology topology;
};

class VkmsDevice
{
public:
    explicit VkmsDevice(Topology const& topology);
    ~VkmsDevice();

    VkmsDevice(VkmsDevice const&) = delete;
    auto operator=(VkmsDevice const&) -> VkmsDevice& = delete;

    auto udev_device() const -> std::unique_ptr<mir::udev::Device>;
    auto devnode() const -> std::string const&;
    auto configfs_path() const noexcept -> std::filesystem::path const&;
    void apply(ConnectorStatusChange const& change);

private:
    void disable_and_remove() noexcept;

    Topology topology;
    std::filesystem::path path;
    std::vector<std::filesystem::path> relation_links;
    std::vector<std::filesystem::path> object_directories;
    std::string card_syspath;
    std::string card_devnode;
    dev_t card_devnum{};
    bool instance_created{false};
    bool enabled{false};
};
}

#endif // MIR_TESTS_ATOMIC_KMS_VKMS_DEVICE_H_
