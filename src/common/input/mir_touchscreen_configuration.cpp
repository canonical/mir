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

#include "mir/input/mir_touchscreen_configuration.h"
#include <ostream>

struct MirTouchscreenConfiguration::Implementation
{
    uint32_t output_id{0};
    MirTouchscreenMappingMode mapping_mode{mir_touchscreen_mapping_mode_to_output};

    Implementation(){}
    Implementation(uint32_t output_id, MirTouchscreenMappingMode mode)
        : output_id{output_id}, mapping_mode{mode}
    {}
};

MirTouchscreenConfiguration::MirTouchscreenConfiguration()
    : impl{std::make_unique<Implementation>()}
{
}

MirTouchscreenConfiguration::MirTouchscreenConfiguration(MirTouchscreenConfiguration && conf)
    : impl{std::move(conf.impl)}
{
}

MirTouchscreenConfiguration::MirTouchscreenConfiguration(MirTouchscreenConfiguration const& conf)
    : impl{std::make_unique<Implementation>(*conf.impl)}
{
}

MirTouchscreenConfiguration& MirTouchscreenConfiguration::operator=(MirTouchscreenConfiguration const& conf)
{
    *impl = *conf.impl;
    return *this;
}

MirTouchscreenConfiguration::~MirTouchscreenConfiguration() = default;

MirTouchscreenConfiguration::MirTouchscreenConfiguration(uint32_t output_id, MirTouchscreenMappingMode mode)
       : impl(std::make_unique<Implementation>(output_id, mode))
{}

uint32_t MirTouchscreenConfiguration::output_id() const
{
    return impl->output_id;
}

void MirTouchscreenConfiguration::output_id(uint32_t output_id)
{
    impl->output_id = output_id;
}

MirTouchscreenMappingMode MirTouchscreenConfiguration::mapping_mode() const
{
    return impl->mapping_mode;
}

void MirTouchscreenConfiguration::mapping_mode(MirTouchscreenMappingMode mode)
{
    impl->mapping_mode = mode;
}

bool MirTouchscreenConfiguration::operator==(MirTouchscreenConfiguration const& other) const
{
    return impl->output_id == other.impl->output_id &&
        impl->mapping_mode == other.impl->mapping_mode;
}

bool MirTouchscreenConfiguration::operator!=(MirTouchscreenConfiguration const& other) const
{
    return !(*this == other);
}

std::ostream& operator<<(std::ostream& out, MirTouchscreenConfiguration const& conf)
{
    return out << " mode:" << conf.mapping_mode() << " outputid:" << conf.output_id();
}

