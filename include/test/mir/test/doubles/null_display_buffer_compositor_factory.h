/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_NULL_DISPLAY_BUFFER_COMPOSITOR_FACTORY_H_
#define MIR_TEST_DOUBLES_NULL_DISPLAY_BUFFER_COMPOSITOR_FACTORY_H_

#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/display_buffer_compositor.h"

#include <thread>

namespace mir
{
namespace test
{
namespace doubles
{

class NullDisplayBufferCompositorFactory : public compositor::DisplayBufferCompositorFactory
{
public:
    auto create_compositor_for(graphics::DisplayBuffer&)
        -> std::unique_ptr<compositor::DisplayBufferCompositor> override
    {
        struct NullDisplayBufferCompositor : compositor::DisplayBufferCompositor
        {
            void composite(compositor::SceneElementSequence&&)
            {
                // yield() is needed to ensure reasonable runtime under
                // valgrind for some tests
                std::this_thread::yield();
            }
        };

        auto raw = new NullDisplayBufferCompositor{};
        return std::unique_ptr<NullDisplayBufferCompositor>(raw);
    }
};

}
}
}

#endif /* MIR_TEST_DOUBLES_NULL_DISPLAY_BUFFER_COMPOSITOR_FACTORY_H_ */
