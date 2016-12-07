/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/input/touchscreen_configuration.h"
#include <ostream>

namespace mi = mir::input;

struct mi::TouchscreenConfiguration::Implementation
{
    uint32_t output_id{0};
    MirTouchscreenMappingMode mapping_mode{mir_touchscreen_mapping_mode_to_output};

    Implementation(){}
    Implementation(uint32_t output_id, MirTouchscreenMappingMode mode)
        : output_id{output_id}, mapping_mode{mode}
    {}
};

mi::TouchscreenConfiguration::TouchscreenConfiguration()
    : impl{std::make_unique<Implementation>()}
{
}

mi::TouchscreenConfiguration::TouchscreenConfiguration(TouchscreenConfiguration && conf)
    : impl{std::move(conf.impl)}
{
}

mi::TouchscreenConfiguration::TouchscreenConfiguration(TouchscreenConfiguration const& conf)
    : impl{std::make_unique<Implementation>(*conf.impl)}
{
}

mi::TouchscreenConfiguration& mi::TouchscreenConfiguration::operator=(TouchscreenConfiguration const& conf)
{
    *impl = *conf.impl;
    return *this;
}

mi::TouchscreenConfiguration::~TouchscreenConfiguration() = default;

mi::TouchscreenConfiguration::TouchscreenConfiguration(uint32_t output_id, MirTouchscreenMappingMode mode)
       : impl(std::make_unique<Implementation>(output_id, mode))
{}

uint32_t mi::TouchscreenConfiguration::output_id() const
{
    return impl->output_id;
}

void mi::TouchscreenConfiguration::output_id(uint32_t output_id)
{
    impl->output_id = output_id;
}

MirTouchscreenMappingMode mi::TouchscreenConfiguration::mapping_mode() const
{
    return impl->mapping_mode;
}

void mi::TouchscreenConfiguration::mapping_mode(MirTouchscreenMappingMode mode)
{
    impl->mapping_mode = mode;
}

bool mi::TouchscreenConfiguration::operator==(TouchscreenConfiguration const& other) const
{
    return impl->output_id == other.impl->output_id &&
        impl->mapping_mode == other.impl->mapping_mode;
}

bool mi::TouchscreenConfiguration::operator!=(TouchscreenConfiguration const& other) const
{
    return !(*this == other);
}

std::ostream& mi::operator<<(std::ostream& out, mi::TouchscreenConfiguration const& conf)
{
    return out << " mode:" << conf.mapping_mode() << " outputid:" << conf.output_id();
}

