/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_CLIENT_DISPLAY_CONFIG_H
#define MIR_CLIENT_DISPLAY_CONFIG_H

#include <mir_toolkit/mir_connection.h>
#include <mir_toolkit/mir_display_configuration.h>

#include <functional>
#include <memory>

namespace mir
{
/// Convenient C++ wrappers around the Mir toolkit API.
///
/// These wrappers are intentionally inline adapters: the compiled code depend directly on the Mir toolkit API.
namespace client
{
class DisplayConfig
{
public:
    DisplayConfig() = default;

    explicit DisplayConfig(MirDisplayConfig* config) : self{config, deleter} {}

    explicit DisplayConfig(MirConnection* connection) :
        self{mir_connection_create_display_configuration(connection), deleter} {}

    operator MirDisplayConfig*() { return self.get(); }

    operator MirDisplayConfig const*() const { return self.get(); }

    void reset() { self.reset(); }

    void for_each_output(std::function<void(MirOutput const*)> const& enumerator) const
    {
        auto const count = mir_display_config_get_num_outputs(*this);

        for (int i = 0; i != count; ++i)
            enumerator(mir_display_config_get_output(*this, i));
    }

    void for_each_output(std::function<void(MirOutput*)> const& enumerator)
    {
        auto const count = mir_display_config_get_num_outputs(*this);

        for (int i = 0; i != count; ++i)
            enumerator(mir_display_config_get_mutable_output(*this, i));
    }

private:
    static void deleter(MirDisplayConfig* config) { mir_display_config_release(config); }

    std::shared_ptr <MirDisplayConfig> self;
};

// Provide a deleted overload to avoid double release "accidents".
void mir_display_config_release(DisplayConfig const& config) = delete;
}
}

#endif //MIR_CLIENT_DISPLAY_CONFIG_H
