/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_HOST_CONNECTION_H_
#define MIR_TEST_DOUBLES_STUB_HOST_CONNECTION_H_

#include "src/server/graphics/nested/host_connection.h"
#include "src/server/graphics/nested/host_surface.h"
#include "src/server/graphics/nested/host_stream.h"
#include "src/server/graphics/nested/host_chain.h"
#include "src/server/graphics/nested/host_surface_spec.h"
#include "src/client/display_configuration.h"
#include "src/include/client/mir/input/input_devices.h"
#include "mir/graphics/platform_operation_message.h"

#include "mir/input/mir_input_config.h"
#include "mir_toolkit/mir_connection.h"
#include <gmock/gmock.h>
#include "mir/test/gmock_fixes.h"

namespace mir
{
namespace test
{
namespace doubles
{

class StubHostConnection : public graphics::nested::HostConnection
{
public:
    StubHostConnection(std::shared_ptr<graphics::nested::HostSurface> const& surf) :
        surface(surf)
    {
    }

    StubHostConnection() :
        StubHostConnection(std::make_shared<NullHostSurface>())
    {
    }

    EGLNativeDisplayType egl_native_display() override { return {}; }

    std::shared_ptr<MirDisplayConfig> create_display_config() override
    {
        auto display_conf = mir::protobuf::DisplayConfiguration{};
        return std::shared_ptr<MirDisplayConfig>{
            new MirDisplayConfig{display_conf}};
    }

    void set_display_config_change_callback(std::function<void()> const&) override
    {
    }

    void apply_display_config(MirDisplayConfig&) override {}

    std::shared_ptr<graphics::nested::HostSurface>
        create_surface(
            std::shared_ptr<graphics::nested::HostStream> const&, geometry::Displacement,
            graphics::BufferProperties, char const*, uint32_t) override
    {
        return surface;
    }

    graphics::PlatformOperationMessage platform_operation(
        unsigned int, graphics::PlatformOperationMessage const&) override
    {
        return {{},{}};
    }

    void set_cursor_image(graphics::CursorImage const&) override
    {
    }
    void hide_cursor() override
    {
    }

    auto graphics_platform_library() -> std::string override { return {}; }

    graphics::nested::UniqueInputConfig create_input_device_config() override
    {
        return graphics::nested::UniqueInputConfig(new MirInputConfig, mir_input_config_release);
    }

    void set_input_device_change_callback(std::function<void(graphics::nested::UniqueInputConfig)> const&) override
    {
    }
    void set_input_event_callback(std::function<void(MirEvent const&, mir::geometry::Rectangle const&)> const&) override
    {
    }
    void emit_input_event(MirEvent const&, mir::geometry::Rectangle const&) override
    {
    }

    std::unique_ptr<graphics::nested::HostStream> create_stream(graphics::BufferProperties const&) const override
    {
        struct NullStream : graphics::nested::HostStream
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            MirRenderSurface* rs() const override { return nullptr; }
#pragma GCC diagnostic pop
            MirBufferStream* handle() const override { return nullptr; }
            EGLNativeWindowType egl_native_window() const override { return 0; }
        };
        return std::make_unique<NullStream>();
    }

    std::unique_ptr<graphics::nested::HostChain> create_chain() const override
    {
        struct NullHostChain : graphics::nested::HostChain
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            MirRenderSurface* rs() const override { return nullptr; }
#pragma GCC diagnostic pop
            void submit_buffer(graphics::nested::NativeBuffer&) override {}
            MirPresentationChain* handle() override { return nullptr; }
            void set_submission_mode(graphics::nested::SubmissionMode) override {}
        };
        return std::make_unique<NullHostChain>();
    }

    class NullHostSurface : public graphics::nested::HostSurface
    {
    public:
        EGLNativeWindowType egl_native_window() override { return {}; }
        void set_event_handler(MirWindowEventCallback, void*) override {}
        void apply_spec(graphics::nested::HostSurfaceSpec&) override {}
    };
    std::shared_ptr<graphics::nested::HostSurface> const surface;
    
    std::shared_ptr<graphics::nested::NativeBuffer> create_buffer(graphics::BufferProperties const&) override
    {
        return nullptr;
    }
    std::shared_ptr<graphics::nested::NativeBuffer> create_buffer(geometry::Size, MirPixelFormat) override
    {
        return nullptr;
    }
    std::shared_ptr<graphics::nested::NativeBuffer> create_buffer(geometry::Size, uint32_t, uint32_t) override
    {
        return nullptr;
    }

    std::unique_ptr<graphics::nested::HostSurfaceSpec> create_surface_spec() override
    {
        struct NullSpec : graphics::nested::HostSurfaceSpec
        {
            void add_chain(graphics::nested::HostChain&, geometry::Displacement, geometry::Size) override {}
            void add_stream(graphics::nested::HostStream&, geometry::Displacement, geometry::Size) override {}
            MirWindowSpec* handle() override { return nullptr; }
        }; 
        return std::make_unique<NullSpec>();
    }
    bool supports_passthrough(graphics::BufferUsage) override
    {
        return true;
    }
    void apply_input_configuration(MirInputConfig const*) override
    {
    }
    optional_value<std::shared_ptr<graphics::MesaAuthExtension>> auth_extension() override
    {
        return {};
    }
    optional_value<std::shared_ptr<graphics::SetGbmExtension>> set_gbm_extension() override
    {
        return {};
    }
    optional_value<mir::Fd> drm_fd() override
    {
        return {};
    }
    void* request_interface(char const*, int) { return nullptr; }
    std::vector<mir::ExtensionDescription> extensions() const override { return {}; }
};

struct MockHostConnection : StubHostConnection
{
    MOCK_METHOD1(set_input_device_change_callback, void (std::function<void(graphics::nested::UniqueInputConfig)> const&));
    MOCK_METHOD1(set_input_event_callback, void (std::function<void(MirEvent const&, mir::geometry::Rectangle const&)> const&));
    MOCK_CONST_METHOD0(create_chain, std::unique_ptr<graphics::nested::HostChain>());
    MOCK_CONST_METHOD1(create_stream, std::unique_ptr<graphics::nested::HostStream>(graphics::BufferProperties const&));
    MOCK_METHOD5(create_surface, std::shared_ptr<graphics::nested::HostSurface>
        (std::shared_ptr<graphics::nested::HostStream> const&, geometry::Displacement,
         graphics::BufferProperties, char const*, uint32_t));
    MOCK_METHOD2(request_interface, void*(char const*, int));
    MOCK_METHOD1(apply_input_configuration, void(MirInputConfig const*));

    void emit_input_event(MirEvent const& event, mir::geometry::Rectangle const& source_frame)
    {
        if (event_callback)
            event_callback(event, source_frame);
    }
    std::vector<ExtensionDescription> extensions() const { return {}; }

    MockHostConnection()
    {
        using namespace testing;
        ON_CALL(*this, set_input_device_change_callback(_))
            .WillByDefault(SaveArg<0>(&device_change_callback));
        ON_CALL(*this, set_input_event_callback(_))
            .WillByDefault(SaveArg<0>(&event_callback));
    }

    std::function<void(graphics::nested::UniqueInputConfig)> device_change_callback;
    std::function<void(MirEvent const&, mir::geometry::Rectangle const&)> event_callback;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_HOST_CONNECTION_H_ */
