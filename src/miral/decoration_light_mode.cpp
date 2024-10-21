#include "miral/decoration_light_mode.h"

#include "mir/decoration/basic_decoration.h"
#include "mir/decoration/theme.h"

#include "mir/decoration/basic_manager.h"
#include "mir/executor.h"
#include "mir/main_loop.h"
#include "mir/server.h"

#include <memory>

namespace msd = mir::shell::decoration;

namespace miral::decoration
{
class LightModeDecoration : public mir::shell::decoration::BasicDecoration
{
};

class LightModeManager : public msd::BasicManager
{
    public:
        LightModeManager(
            mir::ObserverRegistrar<mir::graphics::DisplayConfigurationObserver>& display_configuration_observers,
            std::shared_ptr<mir::graphics::GraphicBufferAllocator> const& buffer_allocator,
            std::shared_ptr<mir::Executor> const& executor,
            std::shared_ptr<mir::input::CursorImages> const& cursor_images) :
            msd::BasicManager(display_configuration_observers, buffer_allocator, executor, cursor_images)
        {
        }

        ~LightModeManager() = default;

    protected:
        virtual std::unique_ptr<msd::Decoration> create_decoration(
            std::shared_ptr<mir::shell::Shell> const locked_shell, std::shared_ptr<mir::scene::Surface> surface)
        {
            return std::make_unique<msd::BasicDecoration>(
                locked_shell,
                buffer_allocator,
                executor,
                cursor_images,
                surface,
                msd::Theme{msd::color(0xBB, 0xBB, 0xBB, 0xFF), msd::color(0x44, 0x44, 0x44, 0xFF)},
                msd::Theme{msd::color(0x77, 0x77, 0x77, 0xFF), msd::color(0x22, 0x22, 0x22, 0xFF)});
    }
};
}

void miral::decoration::LightMode::operator()(mir::Server& server) const
{
    server.add_pre_init_callback(
        [&server]()
        {
            server.set_the_decoration_manager(std::make_shared<miral::decoration::LightModeManager>(
                *server.the_display_configuration_observer_registrar(),
                server.the_buffer_allocator(),
                server.the_main_loop(),
                server.the_cursor_images()));
        });
}
