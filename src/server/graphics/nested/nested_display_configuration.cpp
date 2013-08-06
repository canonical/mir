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

NullDisplayConfiguration::~NullDisplayConfiguration()
{
}

void NullDisplayConfiguration::for_each_card(std::function<void(mg::DisplayConfigurationCard const&)>) const
{
}

void NullDisplayConfiguration::for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)>) const
{
}

void NullDisplayConfiguration::configure_output(mg::DisplayConfigurationOutputId /*id*/, bool /*used*/, geom::Point /*top_left*/, size_t /*mode_index*/)
{
}
