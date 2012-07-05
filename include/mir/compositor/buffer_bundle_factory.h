/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by:
 *  Alan Griffiths <alan@octopull.co.uk>
 *  Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_COMPOSITOR_BUFFER_BUNDLE_FACTORY_H_
#define MIR_COMPOSITOR_BUFFER_BUNDLE_FACTORY_H_

#include "mir/compositor/buffer.h"
#include "mir/geometry/dimensions.h"

#include <memory>

namespace mir
{
namespace compositor
{

class BufferBundle;

class BufferBundleFactory
{
 public:
    virtual ~BufferBundleFactory() {}

    virtual std::shared_ptr<BufferBundle> create_buffer_bundle(
        geometry::Width width,
        geometry::Height height,
        PixelFormat pf) = 0;

    virtual void destroy_buffer_bundle(
        std::shared_ptr<BufferBundle> bundle) = 0;
    
 protected:
    BufferBundleFactory() = default;
    BufferBundleFactory(const BufferBundleFactory&) = delete;
    BufferBundleFactory& operator=(const BufferBundleFactory&) = delete;
};

}
}

#endif // MIR_COMPOSITOR_BUFFER_BUNDLE_FACTORY_H_
