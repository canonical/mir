/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_FRAMEWORK_HEADLESS_DISPLAY_BUFFER_COMPOSITOR_FACTORY_H_
#define MIR_TEST_FRAMEWORK_HEADLESS_DISPLAY_BUFFER_COMPOSITOR_FACTORY_H_
#include "mir/compositor/display_buffer_compositor_factory.h"

namespace mir_test_framework
{
class PassthroughTracker;
struct HeadlessDisplayBufferCompositorFactory : mir::compositor::DisplayBufferCompositorFactory
{
    HeadlessDisplayBufferCompositorFactory();
    HeadlessDisplayBufferCompositorFactory(std::shared_ptr<PassthroughTracker> const& tracker);
    std::unique_ptr<mir::compositor::DisplayBufferCompositor> create_compositor_for(
        mir::graphics::DisplayBuffer& display_buffer) override;
private:
    std::shared_ptr<PassthroughTracker> const tracker;
};
}
#endif /* MIR_TEST_FRAMEWORK_HEADLESS_DISPLAY_BUFFER_COMPOSITOR_FACTORY_H_ */
