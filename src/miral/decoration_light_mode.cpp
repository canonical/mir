/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "miral/decoration_light_mode.h"
#include "miral/basic_decoration/basic_manager.h"
#include "miral/basic_decoration/basic_decoration.h"


#include "mir/executor.h"
#include "mir/main_loop.h"
#include "mir/server.h"

#include <memory>

namespace msd = mir::shell::decoration;
namespace md = miral::decoration;

namespace miral::decoration
{
class LightModeManager : public md::BasicManager
{
    public:
        LightModeManager(
            mir::ObserverRegistrar<mir::graphics::DisplayConfigurationObserver>& display_configuration_observers,
            std::shared_ptr<mir::graphics::GraphicBufferAllocator> const& buffer_allocator,
            std::shared_ptr<mir::Executor> const& executor,
            std::shared_ptr<mir::input::CursorImages> const& cursor_images) :
            md::BasicManager(display_configuration_observers, buffer_allocator, executor, cursor_images)
        {
        }

        virtual ~LightModeManager() = default;

    protected:
        virtual std::unique_ptr<msd::Decoration> create_decoration(
            std::shared_ptr<mir::shell::Shell> const locked_shell, std::shared_ptr<mir::scene::Surface> surface)
        {
            uint32_t const default_normal_button = md::color(0x80, 0x80, 0x80);
            uint32_t const default_active_button = md::color(0xA0, 0xA0, 0xA0);
            uint32_t const default_button_icon = md::color(0xFF, 0xFF, 0xFF);

            static md::ButtonTheme const button_theme = {
                {
                    md::ButtonFunction::Close,
                    {md::color(0xC0, 0x60, 0x60), md::color(0xE0, 0x80, 0x80), md::color(0xFF, 0xFF, 0xFF)},
                },
                {
                    md::ButtonFunction::Maximize,
                    {default_normal_button, default_active_button, default_button_icon},
                },
                {
                    md::ButtonFunction::Minimize,
                    {default_normal_button, default_active_button, default_button_icon},
                },
            };

            return std::make_unique<md::BasicDecoration>(
                locked_shell,
                buffer_allocator,
                executor,
                cursor_images,
                surface,
                md::Theme{md::color(0xBB, 0xBB, 0xBB, 0xFF), md::color(0x44, 0x44, 0x44, 0xFF)},
                md::Theme{md::color(0x77, 0x77, 0x77, 0xFF), md::color(0x22, 0x22, 0x22, 0xFF)},
                button_theme
            )->to_adapter();
    }
};
}

void miral::decoration::LightMode::operator()(mir::Server& server) const
{
    server.add_pre_init_callback(
        [&server]()
        {
            server.set_the_decoration_manager_init(
                [server]
                {
                    return std::make_shared<miral::decoration::LightModeManager>(
                               *server.the_display_configuration_observer_registrar(),
                               server.the_buffer_allocator(),
                               server.the_main_loop(),
                               server.the_cursor_images())
                        ->to_adapter();
                });
        });
}
