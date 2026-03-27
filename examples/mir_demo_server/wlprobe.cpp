/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#include "wlprobe.h"

#include <wayland-client.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <map>

struct mir::examples::WlProbe::Self
{
    std::string const output_file;
    std::map<std::string, uint32_t> globals;  // interface -> highest version
    wl_display* display{nullptr};

    explicit Self(std::string output_file)
        : output_file{std::move(output_file)}
    {
    }

    static void handle_global(
        void* data,
        wl_registry* /*registry*/,
        uint32_t /*id*/,
        char const* interface,
        uint32_t version)
    {
        auto* self = static_cast<Self*>(data);
        // Only store the highest version for each interface
        auto& stored_version = self->globals[interface];
        stored_version = std::max(stored_version, version);
    }

    static void handle_global_remove(
        void* /*data*/,
        wl_registry* /*registry*/,
        uint32_t /*name*/)
    {
        // We don't care about removals during our brief connection
    }

    void query_globals()
    {
        static wl_registry_listener const registry_listener = {
            handle_global,
            handle_global_remove
        };

        auto* registry = wl_display_get_registry(display);
        wl_registry_add_listener(registry, &registry_listener, this);

        // First roundtrip populates globals
        wl_display_roundtrip(display);

        // Second roundtrip ensures all globals are fully initialized
        wl_display_roundtrip(display);

        wl_registry_destroy(registry);
    }

    void write_json() const
    {
        std::ofstream out(output_file);
        if (!out)
        {
            throw std::runtime_error("Failed to open output file: " + output_file);
        }

        // Get current timestamp in milliseconds since epoch
        auto const now = std::chrono::system_clock::now();
        auto const timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();

        out << "{\n";
        out << "  \"generationTimestamp\": " << timestamp << ",\n";
        out << "  \"globals\": [\n";

        size_t i = 0;
        // Map is already sorted by interface name (key)
        for (auto const& [interface, version] : globals)
        {
            out << "    {\n";
            out << "      \"interface\": \"" << interface << "\",\n";
            out << "      \"version\": " << version << "\n";
            out << "    }";

            if (i + 1 < globals.size())
            {
                out << ",";
            }
            out << "\n";
            ++i;
        }

        out << "  ]\n";
        out << "}\n";

        out.close();
    }
};

mir::examples::WlProbe::WlProbe(std::string const& output_file)
    : self{std::make_shared<Self>(output_file)}
{
}

mir::examples::WlProbe::~WlProbe() = default;

void mir::examples::WlProbe::operator()(wl_display* display)
{
    if (self->output_file.empty())
    {
        return;
    }

    self->display = display;
    self->query_globals();
    self->write_json();
}

void mir::examples::WlProbe::operator()(std::weak_ptr<mir::scene::Session> const& /*session*/)
{
}
