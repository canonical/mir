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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_GRAPHICS_PROXY_OUTPUT_H_
#define MIR_GRAPHICS_PROXY_OUTPUT_H_

#include "mir/graphics/output.h"
#include <memory>

namespace mir { namespace graphics {

/**
 * ProxyOutput is a proxy representing any number of other outputs.
 * It returns compatible attributes that best match the whole set of its child
 * outputs. This is for use in multi-monitor scenarios where you only want
 * to render each frame once, and have it looking correct on multiple outputs.
 *
 * Presently ProxyOutput is used to implement multi-monitor frame timing.
 * In future it could also be used for calculating the best sub-pixel
 * arrangement to match differing display hardware.
 */
class ProxyOutput : public Output
{
public:
    virtual ~ProxyOutput() = default;
    virtual Frame last_frame() const override;

    void add_proxy_to(std::weak_ptr<Output>);
private:
    // TODO: children
};

}} // namespace mir::graphics

#endif // MIR_GRAPHICS_PROXY_OUTPUT_H_
