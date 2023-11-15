/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_FRAMEWORK_HEADLESS_DISPLAY_BUFFER_COMPOSITOR_FACTORY_H_
#define MIR_TEST_FRAMEWORK_HEADLESS_DISPLAY_BUFFER_COMPOSITOR_FACTORY_H_
#include "mir/compositor/display_buffer_compositor_factory.h"

namespace mir::graphics
{
class GLRenderingProvider;
class GLConfig;
}

namespace mir_test_framework
{
class PassthroughTracker;
struct HeadlessDisplayBufferCompositorFactory : mir::compositor::DisplayBufferCompositorFactory
{
    HeadlessDisplayBufferCompositorFactory(
        std::shared_ptr<mir::graphics::GLRenderingProvider> render_platform,
        std::shared_ptr<mir::graphics::GLConfig> gl_config);
    HeadlessDisplayBufferCompositorFactory(
        std::shared_ptr<mir::graphics::GLRenderingProvider> render_platform,
        std::shared_ptr<mir::graphics::GLConfig> gl_config,
        std::shared_ptr<PassthroughTracker> const& tracker);

    std::unique_ptr<mir::compositor::DisplayBufferCompositor> create_compositor_for(
        mir::graphics::DisplaySink& display_sink) override;
private:
    std::shared_ptr<mir::graphics::GLRenderingProvider> const render_platform;
    std::shared_ptr<mir::graphics::GLConfig> const gl_config;
    std::shared_ptr<PassthroughTracker> const tracker;
};
}
#endif /* MIR_TEST_FRAMEWORK_HEADLESS_DISPLAY_BUFFER_COMPOSITOR_FACTORY_H_ */
