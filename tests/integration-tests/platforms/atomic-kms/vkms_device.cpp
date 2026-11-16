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

#include <mir/udev/wrapper.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <thread>
#include <ranges>
#include <unordered_set>
#include <iterator>
#include <unistd.h>

namespace mtv = mir::test::vkms;
namespace fs = std::filesystem;

namespace
{
fs::path const vkms_configfs_root{"/sys/kernel/config/vkms"};
std::atomic<unsigned long> device_counter{0};

void write_attribute(fs::path const& path, std::string const& value)
{
    int const fd = ::open(path.c_str(), O_WRONLY | O_CLOEXEC);
    if (fd < 0)
        throw std::system_error{errno, std::generic_category(), "open " + path.string()};

    auto const written = ::write(fd, value.data(), value.size());
    int const write_error = errno;
    if (::close(fd) < 0 && written == static_cast<ssize_t>(value.size()))
        throw std::system_error{errno, std::generic_category(), "close " + path.string()};
    if (written != static_cast<ssize_t>(value.size()))
    {
        if (written < 0)
            throw std::system_error{write_error, std::generic_category(), "write " + path.string()};
        throw std::runtime_error{"short write to " + path.string()};
    }
}

void require_path(fs::path const& path)
{
    if (!fs::exists(path))
    {
        throw std::runtime_error{
            "VKMS configfs topology/status support is unavailable; missing " + path.string() +
            " (Linux 6.19 or a compatible backport is required)"};
    }
}

bool safe_name(std::string const& name)
{
    return !name.empty() && name != "." && name != ".." &&
        std::ranges::all_of(
            name,
            [](unsigned char ch)
            {
                return std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.';
            });
}

template<typename Object>
void validate_names(std::vector<Object> const& objects, char const* kind)
{
    std::unordered_set<std::string> names;
    for (auto const& object : objects)
    {
        if (!safe_name(object.name))
            throw std::invalid_argument{std::string{kind} + " has a path-unsafe name: " + object.name};
        if (!names.insert(object.name).second)
            throw std::invalid_argument{std::string{"duplicate "} + kind + " name: " + object.name};
    }
}

template<typename Object>
void validate_count(std::vector<Object> const& objects, char const* kind)
{
    if (objects.empty() || objects.size() > 31)
        throw std::invalid_argument{std::string{kind} + " count must be between 1 and 31"};
}


std::unordered_set<std::string> names_of_crtcs(mtv::Topology const& topology)
{
    std::unordered_set<std::string> result;
    for (auto const& crtc : topology.crtcs)
        result.insert(crtc.name);
    return result;
}

std::unordered_set<std::string> names_of_encoders(mtv::Topology const& topology)
{
    std::unordered_set<std::string> result;
    for (auto const& encoder : topology.encoders)
        result.insert(encoder.name);
    return result;
}

void validate_unique_links(
    std::vector<std::string> const& links,
    std::string const& description)
{
    std::unordered_set<std::string> unique;
    for (auto const& link : links)
    {
        if (!unique.insert(link).second)
            throw std::invalid_argument{"duplicate " + description + " link: " + link};
    }
}

void validate_links(mtv::Topology const& topology)
{
    auto const crtc_names = names_of_crtcs(topology);
    auto const encoder_names = names_of_encoders(topology);

    for (auto const& plane : topology.planes)
    {
        if (plane.possible_crtcs.empty())
            throw std::invalid_argument{"plane has no possible CRTC: " + plane.name};
        validate_unique_links(plane.possible_crtcs, "plane " + plane.name + " CRTC");
        for (auto const& crtc : plane.possible_crtcs)
        {
            if (!crtc_names.contains(crtc))
                throw std::invalid_argument{"plane " + plane.name + " links to unknown CRTC " + crtc};
        }
    }

    for (auto const& encoder : topology.encoders)
    {
        if (encoder.possible_crtcs.empty())
            throw std::invalid_argument{"encoder has no possible CRTC: " + encoder.name};
        validate_unique_links(encoder.possible_crtcs, "encoder " + encoder.name + " CRTC");
        for (auto const& crtc : encoder.possible_crtcs)
        {
            if (!crtc_names.contains(crtc))
                throw std::invalid_argument{"encoder " + encoder.name + " links to unknown CRTC " + crtc};
        }
    }

    for (auto const& connector : topology.connectors)
    {
        if (connector.possible_encoders.empty())
            throw std::invalid_argument{"connector has no possible encoder: " + connector.name};
        validate_unique_links(connector.possible_encoders, "connector " + connector.name + " encoder");
        for (auto const& encoder : connector.possible_encoders)
        {
            if (!encoder_names.contains(encoder))
            {
                throw std::invalid_argument{
                    "connector " + connector.name + " links to unknown encoder " + encoder};
            }
        }
    }
}

void validate_routes(mtv::Topology const& topology)
{
    for (auto const& crtc : topology.crtcs)
    {
        auto const assigned_planes = [&](mtv::PlaneType type)
        {
            return std::ranges::count_if(
                topology.planes,
                [&](mtv::Plane const& plane)
                {
                    return plane.type == type &&
                        std::ranges::find(plane.possible_crtcs, crtc.name) != plane.possible_crtcs.end();
                });
        };

        if (assigned_planes(mtv::PlaneType::primary) != 1)
            throw std::invalid_argument{"CRTC must have exactly one reachable primary plane: " + crtc.name};
        if (assigned_planes(mtv::PlaneType::cursor) > 1)
            throw std::invalid_argument{"CRTC has more than one reachable cursor plane: " + crtc.name};
        if (std::ranges::none_of(
                topology.encoders,
                [&](mtv::Encoder const& encoder)
                {
                    return std::ranges::find(encoder.possible_crtcs, crtc.name) != encoder.possible_crtcs.end();
                }))
        {
            throw std::invalid_argument{"CRTC has no reachable encoder: " + crtc.name};
        }
    }
}

void validate(mtv::Topology const& topology)
{
    validate_count(topology.crtcs, "CRTC");
    validate_count(topology.planes, "plane");
    validate_count(topology.encoders, "encoder");
    validate_count(topology.connectors, "connector");
    validate_names(topology.crtcs, "CRTC");
    validate_names(topology.planes, "plane");
    validate_names(topology.encoders, "encoder");
    validate_names(topology.connectors, "connector");
    validate_links(topology);
    validate_routes(topology);
}

bool is_card_sysname(char const* sysname)
{
    if (!sysname)
        return false;

    std::string_view const name{sysname};
    return name.starts_with("card") && name.size() > 4 &&
        std::ranges::all_of(
            name.substr(4),
            [](unsigned char ch)
            {
                return std::isdigit(ch);
            });
}

struct Card
{
    std::string syspath;
    std::string devnode;
    dev_t devnum;
    bool ready;
    bool has_named_parent;
    bool is_vkms;
};

std::vector<Card> enumerate_cards(std::shared_ptr<mir::udev::Context> const& context, std::string const& device_name)
{
    mir::udev::Enumerator cards{context};
    cards.match_subsystem("drm");
    cards.match_sysname("card[0-9]*");
    cards.scan_devices();

    std::vector<Card> result;
    for (auto const& device : cards)
    {
        if (!is_card_sysname(device.sysname()))
            continue;

        bool named_parent{false};
        bool vkms{false};
        for (auto current = device.clone(); current; current = current->parent())
        {
            named_parent = named_parent ||
                (current->sysname() && device_name == current->sysname());
            vkms = vkms ||
                (current->driver() && std::strcmp(current->driver(), "vkms") == 0);
        }

        auto const devnode = device.devnode();
        result.push_back(Card{
            device.syspath() ? device.syspath() : "",
            devnode ? devnode : "",
            device.devnum(),
            device.initialised() && devnode,
            named_parent,
            vkms});
    }
    return result;
}

Card discover_card(
    std::shared_ptr<mir::udev::Context> const& context,
    std::string const& device_name,
    std::unordered_set<std::string> const& cards_before)
{
    auto const deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
    do
    {
        auto const cards = enumerate_cards(context, device_name);
        std::vector<Card> named;
        std::vector<Card> added_vkms;
        std::ranges::copy_if(cards, std::back_inserter(named), [](Card const& card) { return card.has_named_parent; });
        std::ranges::copy_if(
            cards,
            std::back_inserter(added_vkms),
            [&](Card const& card) { return card.is_vkms && !cards_before.contains(card.syspath); });

        if (named.size() > 1)
            throw std::runtime_error{"multiple DRM cards matched VKMS configfs device " + device_name};
        if (named.size() == 1 && named.front().ready)
            return named.front();
        if (named.empty())
        {
            if (added_vkms.size() > 1)
                throw std::runtime_error{"VKMS card fallback found more than one new DRM card"};
            if (added_vkms.size() == 1 && added_vkms.front().ready)
                return added_vkms.front();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds{20});
    } while (std::chrono::steady_clock::now() < deadline);

    throw std::runtime_error{"timed out waiting for initialized DRM card for VKMS device " + device_name};
}
}

mtv::TopologyBuilder& mtv::TopologyBuilder::add_crtc(std::string name, bool writeback)
{
    topology.crtcs.push_back(Crtc{std::move(name), writeback});
    return *this;
}

mtv::TopologyBuilder& mtv::TopologyBuilder::add_plane(
    std::string name,
    PlaneType type,
    std::vector<std::string> possible_crtcs)
{
    topology.planes.push_back(Plane{std::move(name), type, std::move(possible_crtcs)});
    return *this;
}

mtv::TopologyBuilder& mtv::TopologyBuilder::add_encoder(
    std::string name,
    std::vector<std::string> possible_crtcs)
{
    topology.encoders.push_back(Encoder{std::move(name), std::move(possible_crtcs)});
    return *this;
}

mtv::TopologyBuilder& mtv::TopologyBuilder::add_connector(
    std::string name,
    ConnectorStatus status,
    std::vector<std::string> possible_encoders)
{
    topology.connectors.push_back(Connector{std::move(name), status, std::move(possible_encoders)});
    return *this;
}

mtv::Topology mtv::TopologyBuilder::build() const
{
    validate(topology);
    return topology;
}

mtv::VkmsDevice::VkmsDevice(Topology const& topology) : topology{topology}
{
    validate(topology);
    require_path(vkms_configfs_root);

    auto const device_name =
        "mir-vkms-" + std::to_string(::getpid()) + "-" + std::to_string(device_counter.fetch_add(1));
    path = vkms_configfs_root / device_name;

    try
    {
        fs::create_directory(path);
        instance_created = true;
        for (auto const* collection : {"crtcs", "planes", "encoders", "connectors"})
            require_path(path / collection);
        require_path(path / "enabled");

        auto create_object = [this](char const* collection, std::string const& name)
        {
            auto const object = path / collection / name;
            fs::create_directory(object);
            object_directories.push_back(object);
            return object;
        };

        for (auto const& crtc : topology.crtcs)
        {
            auto const object = create_object("crtcs", crtc.name);
            require_path(object / "writeback");
            write_attribute(object / "writeback", crtc.writeback ? "1" : "0");
        }
        for (auto const& plane : topology.planes)
        {
            auto const object = create_object("planes", plane.name);
            require_path(object / "type");
            require_path(object / "possible_crtcs");
            write_attribute(object / "type", std::to_string(static_cast<int>(plane.type)));
        }
        for (auto const& encoder : topology.encoders)
        {
            auto const object = create_object("encoders", encoder.name);
            require_path(object / "possible_crtcs");
        }
        for (auto const& connector : topology.connectors)
        {
            auto const object = create_object("connectors", connector.name);
            require_path(object / "status");
            require_path(object / "possible_encoders");
            write_attribute(object / "status", std::to_string(static_cast<int>(connector.status)));
        }

        auto create_relation = [this](fs::path const& target, fs::path const& link)
        {
            fs::create_symlink(fs::absolute(target), link);
            relation_links.push_back(link);
        };
        for (auto const& plane : topology.planes)
        {
            for (auto const& crtc : plane.possible_crtcs)
            {
                create_relation(
                    path / "crtcs" / crtc,
                    path / "planes" / plane.name / "possible_crtcs" / crtc);
            }
        }
        for (auto const& encoder : topology.encoders)
        {
            for (auto const& crtc : encoder.possible_crtcs)
            {
                create_relation(
                    path / "crtcs" / crtc,
                    path / "encoders" / encoder.name / "possible_crtcs" / crtc);
            }
        }
        for (auto const& connector : topology.connectors)
        {
            for (auto const& encoder : connector.possible_encoders)
            {
                create_relation(
                    path / "encoders" / encoder,
                    path / "connectors" / connector.name / "possible_encoders" / encoder);
            }
        }

        auto const context = std::make_shared<mir::udev::Context>();
        std::unordered_set<std::string> cards_before;
        for (auto const& card : enumerate_cards(context, device_name))
            cards_before.insert(card.syspath);

        write_attribute(path / "enabled", "1");
        enabled = true;

        auto const card = discover_card(context, device_name, cards_before);
        card_syspath = card.syspath;
        card_devnode = card.devnode;
        card_devnum = card.devnum;
    }
    catch (...)
    {
        disable_and_remove();
        throw;
    }
}

mtv::VkmsDevice::~VkmsDevice()
{
    disable_and_remove();
}

std::unique_ptr<mir::udev::Device> mtv::VkmsDevice::udev_device() const
{
    mir::udev::Context context;
    auto device = context.device_from_syspath(card_syspath);
    if (!device || device->devnum() != card_devnum)
        throw std::runtime_error{"VKMS DRM device disappeared: " + card_syspath};
    return device;
}

std::string const& mtv::VkmsDevice::devnode() const
{
    return card_devnode;
}

fs::path const& mtv::VkmsDevice::configfs_path() const noexcept
{
    return path;
}

void mtv::VkmsDevice::apply(ConnectorStatusChange const& change)
{
    auto const connector = std::ranges::find(topology.connectors, change.connector, &Connector::name);
    if (connector == topology.connectors.end())
        throw std::invalid_argument{"unknown VKMS connector: " + change.connector};

    write_attribute(
        path / "connectors" / connector->name / "status",
        std::to_string(static_cast<int>(change.status)));
    connector->status = change.status;
}

void mtv::VkmsDevice::disable_and_remove() noexcept
{
    std::error_code ignored;
    if (enabled)
    {
        try
        {
            write_attribute(path / "enabled", "0");
        }
        catch (...)
        {
        }
        enabled = false;
    }

    for (auto const& link : relation_links | std::views::reverse)
        fs::remove(link, ignored);
    relation_links.clear();

    for (auto const& directory : object_directories | std::views::reverse)
        fs::remove(directory, ignored);
    object_directories.clear();

    if (instance_created)
    {
        fs::remove(path, ignored);
        instance_created = false;
    }
}
