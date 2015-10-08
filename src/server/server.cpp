/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/server.h"

#include "mir/emergency_cleanup.h"
#include "mir/fd.h"
#include "mir/frontend/connector.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/display_buffer.h"
#include "mir/options/default_configuration.h"
#include "mir/renderer/gl/render_target.h"
#include "mir/default_server_configuration.h"
#include "mir/logging/logger.h"
#include "mir/log.h"
#include "mir/main_loop.h"
#include "mir/report_exception.h"
#include "mir/run_mir.h"
#include "mir/cookie_factory.h"

// TODO these are used to frig a stub renderer when running headless
#include "mir/compositor/renderer.h"
#include "mir/graphics/renderable.h"
#include "mir/compositor/renderer_factory.h"

#include <iostream>

namespace mo = mir::options;

#define FOREACH_WRAPPER(MACRO)\
    MACRO(cursor)\
    MACRO(cursor_listener)\
    MACRO(display_buffer_compositor_factory)\
    MACRO(display_configuration_policy)\
    MACRO(shell)

#define FOREACH_OVERRIDE(MACRO)\
    MACRO(compositor)\
    MACRO(cursor_images)\
    MACRO(display_buffer_compositor_factory)\
    MACRO(display_configuration_report)\
    MACRO(gl_config)\
    MACRO(host_lifecycle_event_listener)\
    MACRO(input_dispatcher)\
    MACRO(logger)\
    MACRO(prompt_session_listener)\
    MACRO(prompt_session_manager)\
    MACRO(server_status_listener)\
    MACRO(session_authorizer)\
    MACRO(session_listener)\
    MACRO(session_mediator_report)\
    MACRO(shell)\
    MACRO(application_not_responding_detector)

#define FOREACH_ACCESSOR(MACRO)\
    MACRO(the_buffer_stream_factory)\
    MACRO(the_compositor)\
    MACRO(the_composite_event_filter)\
    MACRO(the_cursor_listener)\
    MACRO(the_cursor)\
    MACRO(the_display)\
    MACRO(the_display_configuration_controller)\
    MACRO(the_focus_controller)\
    MACRO(the_gl_config)\
    MACRO(the_graphics_platform)\
    MACRO(the_input_targeter)\
    MACRO(the_logger)\
    MACRO(the_main_loop)\
    MACRO(the_prompt_session_listener)\
    MACRO(the_session_authorizer)\
    MACRO(the_session_coordinator)\
    MACRO(the_session_listener)\
    MACRO(the_surface_factory)\
    MACRO(the_prompt_session_manager)\
    MACRO(the_shell)\
    MACRO(the_shell_display_layout)\
    MACRO(the_surface_coordinator)\
    MACRO(the_touch_visualizer)\
    MACRO(the_input_device_hub)\
    MACRO(the_application_not_responding_detector)

#define MIR_SERVER_BUILDER(name)\
    std::function<std::result_of<decltype(&mir::DefaultServerConfiguration::the_##name)(mir::DefaultServerConfiguration*)>::type()> name##_builder;

#define MIR_SERVER_WRAPPER(name)\
    std::function<std::result_of<decltype(&mir::DefaultServerConfiguration::the_##name)(mir::DefaultServerConfiguration*)>::type\
        (std::result_of<decltype(&mir::DefaultServerConfiguration::the_##name)(mir::DefaultServerConfiguration*)>::type const&)> name##_wrapper;

struct mir::Server::Self
{
    bool exit_status{false};
    std::weak_ptr<options::Option> options;
    std::string config_file;
    std::shared_ptr<ServerConfiguration> server_config;
    std::shared_ptr<cookie::CookieFactory> cookie_factory;

    std::function<void()> init_callback{[]{}};
    int argc{0};
    char const** argv{nullptr};
    std::function<void()> exception_handler{};
    Terminator terminator{};
    EmergencyCleanupHandler emergency_cleanup_handler;

    std::function<void(int argc, char const* const* argv)> command_line_hander{};

    /// set a callback to introduce additional configuration options.
    /// this will be invoked by run() before server initialisation starts
    void set_add_configuration_options(
        std::function<void(options::DefaultConfiguration& config)> const& add_configuration_options);

    std::function<void(options::DefaultConfiguration& config)> add_configuration_options{
        [](options::DefaultConfiguration&){}};

    FOREACH_OVERRIDE(MIR_SERVER_BUILDER)

    shell::WindowManagerBuilder wmb;

    FOREACH_WRAPPER(MIR_SERVER_WRAPPER)
};

#undef MIR_SERVER_BUILDER
#undef MIR_SERVER_WRAPPER

#define MIR_SERVER_CONFIG_OVERRIDE(name)\
auto the_##name()\
-> decltype(mir::DefaultServerConfiguration::the_##name()) override\
{\
    if (self->name##_builder)\
    {\
        if (auto const result = name([this]{ return self->name##_builder(); }))\
            return result;\
    }\
\
    return mir::DefaultServerConfiguration::the_##name();\
}

#define MIR_SERVER_CONFIG_WRAP(name)\
auto wrap_##name(decltype(Self::name##_wrapper)::result_type const& wrapped)\
-> decltype(mir::DefaultServerConfiguration::wrap_##name({})) override\
{\
    if (self->name##_wrapper)\
        return name(\
            [&] { return self->name##_wrapper(wrapped); });\
\
    return mir::DefaultServerConfiguration::wrap_##name(wrapped);\
}

// TODO these are used to frig a stub renderer when running headless
namespace
{
class StubGLRenderer : public mir::compositor::Renderer
{
public:
    StubGLRenderer(mir::graphics::DisplayBuffer& display_buffer)
        : render_target{
            dynamic_cast<mir::renderer::gl::RenderTarget*>(
                display_buffer.native_display_buffer())}
    {
    }

    void set_viewport(mir::geometry::Rectangle const&) override {}

    void set_rotation(float) override {}

    void render(mir::graphics::RenderableList const& renderables) const override
    {
        // Invoke GL renderer specific functions if the DisplayBuffer supports them
        if (render_target)
            render_target->make_current();

        for (auto const& r : renderables)
            r->buffer(); // We need to consume a buffer to unblock client tests

        // Invoke GL renderer specific functions if the DisplayBuffer supports them
        if (render_target)
            render_target->swap_buffers();
    }

    void suspend() override {}

    mir::renderer::gl::RenderTarget* const render_target;
};

class StubGLRendererFactory : public mir::compositor::RendererFactory
{
public:
    auto create_renderer_for(mir::graphics::DisplayBuffer& display_buffer)
    -> std::unique_ptr<mir::compositor::Renderer>
    {
        return std::make_unique<StubGLRenderer>(display_buffer);
    }
};
}

struct mir::Server::ServerConfiguration : mir::DefaultServerConfiguration
{
    ServerConfiguration(
        std::shared_ptr<options::Configuration> const& configuration_options,
        std::shared_ptr<Self> const& self) :
        DefaultServerConfiguration(configuration_options),
        self(self.get())
    {
    }

    // TODO this is an ugly frig to avoid exposing the render factory to end users and tests running headless
    auto the_renderer_factory() -> std::shared_ptr<compositor::RendererFactory> override
    {
        auto const& options = the_options();
        if (options->is_set(options::platform_graphics_lib))
        {
            auto const graphics_lib = options->get<std::string>(options::platform_graphics_lib);

            if (graphics_lib.find("graphics-dummy.so") != std::string::npos)
                return std::make_shared<StubGLRendererFactory>();
        }
        return mir::DefaultServerConfiguration::the_renderer_factory();
    }

    auto the_cookie_factory() -> std::shared_ptr<cookie::CookieFactory> override
    {
        if (self->cookie_factory)
        {
            return self->cookie_factory;
        }
        return mir::DefaultServerConfiguration::the_cookie_factory();
    }

    using mir::DefaultServerConfiguration::the_options;

    FOREACH_OVERRIDE(MIR_SERVER_CONFIG_OVERRIDE)

    FOREACH_WRAPPER(MIR_SERVER_CONFIG_WRAP)

    auto the_window_manager_builder() -> shell::WindowManagerBuilder override
    {
        if (self->wmb) return self->wmb;
        return mir::DefaultServerConfiguration::the_window_manager_builder();
    }

    Self* const self;
};

#undef MIR_SERVER_CONFIG_OVERRIDE
#undef MIR_SERVER_CONFIG_WRAP

namespace
{
std::shared_ptr<mo::DefaultConfiguration> configuration_options(
    int argc,
    char const** argv,
    std::function<void(int argc, char const* const* argv)> const& command_line_hander,
    std::string const& config_file)
{
    std::shared_ptr<mo::DefaultConfiguration> result;

    if (command_line_hander)
        result = std::make_shared<mo::DefaultConfiguration>(argc, argv, command_line_hander, config_file);
    else
        result = std::make_shared<mo::DefaultConfiguration>(argc, argv, config_file);

    return result;
}

template<typename ConfigPtr>
void verify_setting_allowed(ConfigPtr const& initialized)
{
    if (initialized)
       BOOST_THROW_EXCEPTION(std::logic_error("Cannot amend configuration after apply_settings() call"));
}

template<typename ConfigPtr>
void verify_accessing_allowed(ConfigPtr const& initialized)
{
    if (!initialized)
       BOOST_THROW_EXCEPTION(std::logic_error("Cannot use configuration before apply_settings() call"));
}
}

mir::Server::Server() :
    self(std::make_shared<Self>())
{
}

void mir::Server::Self::set_add_configuration_options(
    std::function<void(mo::DefaultConfiguration& config)> const& add_configuration_options)
{
    this->add_configuration_options = add_configuration_options;
}


void mir::Server::set_command_line(int argc, char const* argv[])
{
    verify_setting_allowed(self->server_config);
    self->argc = argc;
    self->argv = argv;
}

void mir::Server::override_the_cookie_factory(mir::cookie::Secret const& secret)
{
    verify_setting_allowed(self->server_config);
    self->cookie_factory = mir::cookie::CookieFactory::create_from_secret(secret);
}

void mir::Server::add_init_callback(std::function<void()> const& init_callback)
{
    auto const& existing = self->init_callback;

    auto const updated = [=]
        {
            existing();
            init_callback();
        };

    self->init_callback = updated;
}

void mir::Server::set_command_line_handler(
    std::function<void(int argc, char const* const* argv)> const& command_line_hander)
{
    verify_setting_allowed(self->server_config);
    self->command_line_hander = command_line_hander;
}

void mir::Server::set_config_filename(std::string const& config_file)
{
    verify_setting_allowed(self->server_config);
    self->config_file = config_file;
}

auto mir::Server::get_options() const -> std::shared_ptr<options::Option>
{
    verify_accessing_allowed(self->server_config);
    return self->options.lock();
}

void mir::Server::set_exception_handler(std::function<void()> const& exception_handler)
{
    self->exception_handler = exception_handler;
}

void mir::Server::set_terminator(Terminator const& terminator)
{
    self->terminator = terminator;
}

void mir::Server::add_emergency_cleanup(EmergencyCleanupHandler const& handler)
{
    if (auto const& existing = self->emergency_cleanup_handler)
    {
        self->emergency_cleanup_handler = [=]
            {
                existing();
                handler();
            };
    }
    else
    {
        self->emergency_cleanup_handler = handler;
    }
}

void mir::Server::apply_settings()
{
    if (self->server_config) return;

    auto const options = configuration_options(self->argc, self->argv, self->command_line_hander, self->config_file);
    self->add_configuration_options(*options);

    auto const config = std::make_shared<ServerConfiguration>(options, self);
    self->server_config = config;
    self->options = config->the_options();

    mir::logging::set_logger(config->the_logger());
}

void mir::Server::run()
try
{
    mir::log_info("Starting");
    verify_accessing_allowed(self->server_config);

    auto const emergency_cleanup = self->server_config->the_emergency_cleanup();

    if (self->emergency_cleanup_handler)
        emergency_cleanup->add(self->emergency_cleanup_handler);

    run_mir(
        *self->server_config,
        [&](DisplayServer&) { self->init_callback(); },
        self->terminator);

    self->exit_status = true;
    self->server_config.reset();
}
catch (...)
{
    self->server_config = nullptr;

    if (self->exception_handler)
        self->exception_handler();
    else
        mir::report_exception(std::cerr);
}

auto mir::Server::supported_pixel_formats() const -> std::vector<MirPixelFormat>
{
    return self->server_config->the_buffer_allocator()->supported_pixel_formats();
}

void mir::Server::stop()
{
    mir::log_info("Stopping");
    if (self->server_config)
        if (auto const main_loop = the_main_loop())
            main_loop->stop();
}

bool mir::Server::exited_normally()
{
    return self->exit_status;
}

auto mir::Server::open_client_socket() -> Fd
{
    if (auto const config = self->server_config)
        return Fd{config->the_connector()->client_socket_fd()};

    BOOST_THROW_EXCEPTION(std::logic_error("Cannot open connection when not running"));
}

auto mir::Server::open_prompt_socket() -> Fd
{
    if (auto const config = self->server_config)
        return Fd{config->the_prompt_connector()->client_socket_fd()};

    BOOST_THROW_EXCEPTION(std::logic_error("Cannot open connection when not running"));
}

auto mir::Server::open_client_socket(ConnectHandler const& connect_handler) -> Fd
{
    if (auto const config = self->server_config)
        return Fd{config->the_connector()->client_socket_fd(connect_handler)};

    BOOST_THROW_EXCEPTION(std::logic_error("Cannot open connection when not running"));
}

#define MIR_SERVER_ACCESSOR(name)\
auto mir::Server::name() const -> decltype(self->server_config->name())\
{\
    verify_accessing_allowed(self->server_config);\
    return self->server_config->name();\
}

FOREACH_ACCESSOR(MIR_SERVER_ACCESSOR)

#undef MIR_SERVER_ACCESSOR

#define MIR_SERVER_OVERRIDE(name)\
void mir::Server::override_the_##name(decltype(Self::name##_builder) const& value)\
{\
    verify_setting_allowed(self->server_config);\
    self->name##_builder = value;\
}

FOREACH_OVERRIDE(MIR_SERVER_OVERRIDE)

#undef MIR_SERVER_OVERRIDE

void mir::Server::override_the_window_manager_builder(shell::WindowManagerBuilder const wmb)
{
    verify_setting_allowed(self->server_config);
    self->wmb = wmb;
}

#define MIR_SERVER_WRAP(name)\
void mir::Server::wrap_##name(decltype(Self::name##_wrapper) const& value)\
{\
    verify_setting_allowed(self->server_config);\
    self->name##_wrapper = value;\
}

FOREACH_WRAPPER(MIR_SERVER_WRAP)

#undef MIR_SERVER_WRAP

void mir::Server::add_configuration_option(
    std::string const& option,
    std::string const& description,
    int default_)
{
    verify_setting_allowed(self->server_config);
    namespace po = boost::program_options;

    auto const& existing = self->add_configuration_options;

    auto const option_adder = [=](options::DefaultConfiguration& config)
        {
            existing(config);

            config.add_options()
            (option.c_str(), po::value<int>()->default_value(default_), description.c_str());
        };

    self->set_add_configuration_options(option_adder);
}

void mir::Server::add_configuration_option(
    std::string const& option,
    std::string const& description,
    std::string const& default_)
{
    verify_setting_allowed(self->server_config);
    namespace po = boost::program_options;

    auto const& existing = self->add_configuration_options;

    auto const option_adder = [=](options::DefaultConfiguration& config)
        {
            existing(config);

            config.add_options()
            (option.c_str(), po::value<std::string>()->default_value(default_), description.c_str());
        };

    self->set_add_configuration_options(option_adder);
}

void mir::Server::add_configuration_option(
    std::string const& option,
    std::string const& description,
    char const* default_value)
{
    add_configuration_option(option, description, std::string{default_value});
}

void mir::Server::add_configuration_option(
    std::string const& option,
    std::string const& description,
    bool default_)
{
    verify_setting_allowed(self->server_config);
    namespace po = boost::program_options;

    auto const& existing = self->add_configuration_options;

    auto const option_adder = [=](options::DefaultConfiguration& config)
        {
            existing(config);

            config.add_options()
            (option.c_str(), po::value<decltype(default_)>()->default_value(default_), description.c_str());
        };

    self->set_add_configuration_options(option_adder);
}

void mir::Server::add_configuration_option(
    std::string const& option,
    std::string const& description,
    OptionType type)
{
    verify_setting_allowed(self->server_config);
    namespace po = boost::program_options;

    auto const& existing = self->add_configuration_options;

    switch (type)
    {
    case OptionType::null:
        {
            auto const option_adder = [=](options::DefaultConfiguration& config)
                {
                    existing(config);

                    config.add_options()
                    (option.c_str(), description.c_str());
                };

            self->set_add_configuration_options(option_adder);
        }
        break;

    case OptionType::integer:
        {
            auto const option_adder = [=](options::DefaultConfiguration& config)
                {
                    existing(config);

                    config.add_options()
                    (option.c_str(), po::value<int>(), description.c_str());
                };

            self->set_add_configuration_options(option_adder);
        }
        break;

    case OptionType::string:
        {
            auto const option_adder = [=](options::DefaultConfiguration& config)
                {
                    existing(config);

                    config.add_options()
                    (option.c_str(), po::value<std::string>(), description.c_str());
                };

            self->set_add_configuration_options(option_adder);
        }
        break;

    case OptionType::boolean:
        {
            auto const option_adder = [=](options::DefaultConfiguration& config)
                {
                    existing(config);

                    config.add_options()
                    (option.c_str(), po::value<bool>(), description.c_str());
                };

            self->set_add_configuration_options(option_adder);
        }
        break;
    }
}
