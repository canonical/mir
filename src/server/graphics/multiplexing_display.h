/*
 * Copyright © 2022 Canonical Ltd.
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

#include "mir/graphics/display.h"

#include <vector>
#include <memory>

namespace mir::graphics
{
class MultiplexingDisplay : public Display
{
public:
    MultiplexingDisplay(std::vector<std::unique_ptr<Display>> displays);
    ~MultiplexingDisplay() override;

    void for_each_display_sync_group(std::function<void(DisplaySyncGroup&)> const& f) override;

    auto configuration() const -> std::unique_ptr<DisplayConfiguration> override;

    auto apply_if_configuration_preserves_display_buffers(DisplayConfiguration const& conf) -> bool override;

    void configure(DisplayConfiguration const& conf) override;

    void register_configuration_change_handler(
        EventHandlerRegister& handlers,
        DisplayConfigurationChangeHandler const& conf_change_handler) override;

    void register_pause_resume_handlers(
        EventHandlerRegister& handlers,
        DisplayPauseHandler const& pause_handler,
        DisplayResumeHandler const& resume_handler) override;

    void pause() override;

    void resume() override;

    auto create_hardware_cursor() -> std::shared_ptr<Cursor> override;

    auto last_frame_on(unsigned output_id) const -> Frame override;

    auto create_gl_context() const -> std::unique_ptr<renderer::gl::Context> override;
private:
    std::vector<std::unique_ptr<Display>> const displays;
};
}
