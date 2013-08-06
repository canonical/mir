/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/display_configuration.h"

#include <ostream>

namespace mg = mir::graphics;

namespace
{

class StreamPropertiesRecovery
{
public:
    StreamPropertiesRecovery(std::ostream& stream)
        : stream(stream),
          flags{stream.flags()},
          precision{stream.precision()}
    {
    }

    ~StreamPropertiesRecovery()
    {
        stream.precision(precision);
        stream.flags(flags);
    }

private:
    std::ostream& stream;
    std::ios_base::fmtflags const flags;
    std::streamsize const precision;
};

}

std::ostream& mg::operator<<(std::ostream& out, mg::DisplayConfigurationMode const& val)
{
    StreamPropertiesRecovery const stream_properties_recovery{out};

    out.precision(1);
    out.setf(std::ios_base::fixed);

    return out << val.size.width << "x" << val.size.height << "@" << val.vrefresh_hz;
}

std::ostream& mg::operator<<(std::ostream& out, mg::DisplayConfigurationOutput const& val)
{
    out << "{ id: " << val.id << ", card_id: " << val.card_id << " modes: [";

    for (size_t i = 0; i < val.modes.size(); ++i)
    {
        out << val.modes[i];
        if (i != val.modes.size() - 1)
            out << ", ";
    }

    out << "], physical_size_mm: " << val.physical_size_mm.width << "x" << val.physical_size_mm.height;
    out << ", connected: " << (val.connected ? "true" : "false");
    out << ", used: " << (val.used ? "true" : "false");
    out << ", top_left: " << val.top_left;
    out << ", current_mode: " << val.current_mode_index << " (";
    if (val.current_mode_index < val.modes.size())
        out << val.modes[val.current_mode_index];
    else
        out << "none";

    out << ") }";

    return out;
}

bool mg::operator==(mg::DisplayConfigurationMode const& val1,
                    mg::DisplayConfigurationMode const& val2)
{
    return (val1.size == val2.size) &&
           (val1.vrefresh_hz == val2.vrefresh_hz);
}

bool mg::operator!=(mg::DisplayConfigurationMode const& val1,
                    mg::DisplayConfigurationMode const& val2)
{
    return !(val1 == val2);
}

bool mg::operator==(mg::DisplayConfigurationOutput const& val1,
                    mg::DisplayConfigurationOutput const& val2)
{
    bool equal{(val1.id == val2.id) &&
               (val1.card_id == val2.card_id) &&
               (val1.physical_size_mm == val2.physical_size_mm) &&
               (val1.connected == val2.connected) &&
               (val1.used == val2.used) &&
               (val1.top_left == val2.top_left) &&
               (val1.current_mode_index == val2.current_mode_index) &&
               (val1.modes.size() == val2.modes.size())};

    if (equal)
    {
        for (size_t i = 0; i < val1.modes.size(); i++)
        {
            equal = equal && (val1.modes[i] == val2.modes[i]);
            if (!equal) break;
        }
    }

    return equal;
}

bool mg::operator!=(mg::DisplayConfigurationOutput const& val1,
                    mg::DisplayConfigurationOutput const& val2)
{
    return !(val1 == val2);
}
