/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by:
 *  Alan Griffiths <alan@octopull.co.uk>
 *  Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_COMPOSITOR_BUFFER_BUNDLE_FACTORY_H_
#define MIR_COMPOSITOR_BUFFER_BUNDLE_FACTORY_H_

#include <memory>

namespace mir
{
namespace compositor
{
struct BufferProperties;
}

namespace surfaces
{
class BufferBundle;

class BufferBundleFactory
{
public:
    virtual ~BufferBundleFactory() {}

    virtual std::shared_ptr<BufferBundle> create_buffer_bundle(
        compositor::BufferProperties const& buffer_properties) = 0;

protected:
    BufferBundleFactory() = default;
    BufferBundleFactory(const BufferBundleFactory&) = delete;
    BufferBundleFactory& operator=(const BufferBundleFactory&) = delete;
};

}
}

#endif // MIR_COMPOSITOR_BUFFER_BUNDLE_FACTORY_H_
