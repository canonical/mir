/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Eleni Maria Stea <elenimaria.stea@canonical.com>
 */

#include "nested_display_configuration.h"

namespace mgn = mir::graphics::nested;

mgn::NestedDisplayConfiguration::~NestedDisplayConfiguration() noexcept
{
}

void mgn::NestedDisplayConfiguration::for_each_card(std::function<void(DisplayConfigurationCard const&)>) const
{
    // TODO
}

void mgn::NestedDisplayConfiguration::for_each_output(std::function<void(DisplayConfigurationOutput const&)>) const
{
    // TODO
}

void mgn::NestedDisplayConfiguration::configure_output(DisplayConfigurationOutputId /*id*/, bool /*used*/, geometry::Point /*top_left*/, size_t /*mode_index*/)
{
    // TODO
}
