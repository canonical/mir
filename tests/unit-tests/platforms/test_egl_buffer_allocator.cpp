/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3,
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

#include "src/platforms/renderer-generic-egl/buffer_allocator.h"
#ifdef MIR_BUILD_PLATFORM_GBM_KMS
#include "src/platforms/gbm-kms/server/buffer_allocator.h"
#include "src/platforms/gbm-kms/server/surfaceless_egl_context.h"
#endif
#include "mir/anonymous_shm_file.h"
#include "mir/graphics/egl_context_executor.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/texture.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/test/doubles/explicit_executor.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/null_gl_context.h"
#include "mir/wayland/client.h"
#include "linux-dmabuf-client.h"

#include <drm_fourcc.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <poll.h>
#include <sys/socket.h>
#include <wayland-client.h>
#include <wayland-server.h>

namespace mg = mir::graphics;
namespace mgc = mg::common;
namespace mtd = mir::test::doubles;
namespace mw = mir::wayland;
using namespace testing;

namespace
{
class TestClient : public mw::Client
{
public:
    explicit TestClient(wl_client* client) : client{client} {}
    ~TestClient() override { unregister_client(client); }
    void register_self(std::shared_ptr<TestClient> const& self) { register_client(client, self); }
    auto raw_client() const -> wl_client* override { return client; }
    auto is_being_destroyed() const -> bool override { return false; }
    MOCK_METHOD(std::shared_ptr<mir::scene::Session>, client_session, (), (const, override));
    MOCK_METHOD(uint32_t, next_serial, (std::shared_ptr<MirEvent const>), (override));
    MOCK_METHOD(std::optional<std::shared_ptr<MirEvent const>>, event_for, (uint32_t), (override));
    MOCK_METHOD(void, set_output_geometry_scale, (float), (override));
    MOCK_METHOD(float, output_geometry_scale, (), (override));
private:
    wl_client* const client;
};

EGLBoolean query_formats(EGLDisplay, EGLint count, EGLint* formats, EGLint* num_formats)
{
    *num_formats = 1;
    if (count) formats[0] = DRM_FORMAT_ARGB8888;
    return EGL_TRUE;
}

EGLBoolean query_modifiers(
    EGLDisplay, EGLint, EGLint count, EGLuint64KHR* modifiers, EGLBoolean* external_only, EGLint* num_modifiers)
{
    *num_modifiers = 1;
    if (count)
    {
        modifiers[0] = DRM_FORMAT_MOD_LINEAR;
        external_only[0] = EGL_FALSE;
    }
    return EGL_TRUE;
}

enum class Allocator
{
    generic,
#ifdef MIR_BUILD_PLATFORM_GBM_KMS
    gbm,
#endif
};

auto const allocator_types = Values(
    Allocator::generic
#ifdef MIR_BUILD_PLATFORM_GBM_KMS
    , Allocator::gbm
#endif
);
}

class EGLBufferAllocatorTest : public TestWithParam<Allocator>
{
protected:
    void SetUp() override
    {
        mock_egl.provide_egl_extensions();
        mock_gl.provide_gles_extensions();
        ON_CALL(mock_egl, eglQueryString(_, EGL_EXTENSIONS))
            .WillByDefault(Invoke([this](EGLDisplay, EGLint) { return extensions.c_str(); }));
        ON_CALL(mock_gl, glCreateShader(_)).WillByDefault(Return(1));
        ON_CALL(mock_gl, glCreateProgram()).WillByDefault(Return(2));

        display.reset(wl_display_create());
        ASSERT_NE(nullptr, display);
        int sockets[2];
        ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sockets));
        raw_client = wl_client_create(display.get(), sockets[0]);
        ASSERT_NE(nullptr, raw_client);
        client = std::make_shared<NiceMock<TestClient>>(raw_client);
        client->register_self(client);
        client_display.reset(wl_display_connect_to_fd(sockets[1]));
        ASSERT_NE(nullptr, client_display);
        resource = wl_resource_create(raw_client, &wl_buffer_interface, 1, 0);
        ASSERT_NE(nullptr, resource);
    }

    void TearDown() override
    {
        renderer.reset();
        allocator.reset();
        wayland_executor->execute();
        provider.reset();
        egl_executor.reset();
        if (display) wl_display_destroy_clients(display.get());
        client.reset();
        client_display.reset();
        display.reset();
    }

    void create_allocator(bool with_dmabuf = false)
    {
        egl_executor = std::make_shared<mgc::EGLContextExecutor>(std::make_unique<mtd::NullGLContext>());
        if (with_dmabuf)
        {
            using Function = mtd::MockEGL::generic_function_pointer_t;
            ON_CALL(mock_egl, eglGetProcAddress(StrEq("eglQueryDmaBufFormatsEXT")))
                .WillByDefault(Return(reinterpret_cast<Function>(&query_formats)));
            ON_CALL(mock_egl, eglGetProcAddress(StrEq("eglQueryDmaBufModifiersEXT")))
                .WillByDefault(Return(reinterpret_cast<Function>(&query_modifiers)));
            provider = std::make_shared<mg::DMABufEGLProvider>(
                mock_egl.fake_egl_display,
                std::make_shared<mg::EGLExtensions>(),
                mg::EGLExtensions::EXTImageDmaBufImportModifiers{mock_egl.fake_egl_display},
                egl_executor,
                [](mg::DRMFormat, std::span<uint64_t const>, mir::geometry::Size) -> std::shared_ptr<mg::DMABufBuffer>
                {
                    ADD_FAILURE() << "Importing a client buffer should not allocate a dmabuf";
                    return nullptr;
                });
        }
#ifdef MIR_BUILD_PLATFORM_GBM_KMS
        if (GetParam() == Allocator::gbm)
        {
            allocator = std::make_unique<mg::gbm::BufferAllocator>(
                std::make_unique<mg::gbm::SurfacelessEGLContext>(mock_egl.fake_egl_display),
                egl_executor,
                provider);
            renderer = std::make_unique<mg::gbm::GLRenderingProvider>(
                nullptr, egl_executor, provider, mock_egl.fake_egl_display, mock_egl.fake_egl_context);
        }
        else
#endif
        {
            allocator = std::make_unique<mg::egl::generic::BufferAllocator>(
                mock_egl.fake_egl_display, mock_egl.fake_egl_context, provider);
            renderer = std::make_unique<mg::egl::generic::GLRenderingProvider>(
                mock_egl.fake_egl_display, mock_egl.fake_egl_context, provider);
        }
        allocator->bind_display(display.get(), wayland_executor);
    }

    void roundtrip()
    {
        bool done = false;
        auto* callback = wl_display_sync(client_display.get());
        static wl_callback_listener const listener{
            [](void* data, wl_callback*, uint32_t) { *static_cast<bool*>(data) = true; }};
        wl_callback_add_listener(callback, &listener, &done);
        ASSERT_GE(wl_display_flush(client_display.get()), 0);
        ASSERT_GE(wl_event_loop_dispatch(wl_display_get_event_loop(display.get()), 0), 0);
        wl_display_flush_clients(display.get());
        pollfd fd{wl_display_get_fd(client_display.get()), POLLIN, 0};
        ASSERT_EQ(1, poll(&fd, 1, 1000));
        ASSERT_GE(wl_display_dispatch(client_display.get()), 0);
        EXPECT_TRUE(done);
        wl_callback_destroy(callback);
    }

    void create_dmabuf()
    {
        uint32_t global_name = 0;
        auto* registry = wl_display_get_registry(client_display.get());
        static wl_registry_listener const listener{
            [](void* data, wl_registry*, uint32_t name, char const* interface, uint32_t)
            {
                if (std::string{interface} == zwp_linux_dmabuf_v1_interface.name)
                    *static_cast<uint32_t*>(data) = name;
            },
            [](void*, wl_registry*, uint32_t) {}};
        wl_registry_add_listener(registry, &listener, &global_name);
        ASSERT_NO_FATAL_FAILURE(roundtrip());
        ASSERT_NE(0u, global_name);
        auto* dmabuf = static_cast<zwp_linux_dmabuf_v1*>(
            wl_registry_bind(registry, global_name, &zwp_linux_dmabuf_v1_interface, 3));
        static zwp_linux_dmabuf_v1_listener const dmabuf_listener{
            [](void*, zwp_linux_dmabuf_v1*, uint32_t) {},
            [](void*, zwp_linux_dmabuf_v1*, uint32_t, uint32_t, uint32_t) {}};
        zwp_linux_dmabuf_v1_add_listener(dmabuf, &dmabuf_listener, nullptr);
        auto* params = zwp_linux_dmabuf_v1_create_params(dmabuf);
        mir::AnonymousShmFile const backing{64};
        zwp_linux_buffer_params_v1_add(params, backing.fd(), 0, 0, 16, 0, DRM_FORMAT_MOD_LINEAR);
        auto* buffer = zwp_linux_buffer_params_v1_create_immed(params, 4, 4, DRM_FORMAT_ARGB8888, 0);
        ASSERT_NO_FATAL_FAILURE(roundtrip());
        resource = wl_client_get_object(raw_client, wl_proxy_get_id(reinterpret_cast<wl_proxy*>(buffer)));
        ASSERT_NE(nullptr, resource);
        wl_buffer_destroy(buffer);
        zwp_linux_buffer_params_v1_destroy(params);
        zwp_linux_dmabuf_v1_destroy(dmabuf);
        wl_registry_destroy(registry);
    }

    void expect_no_legacy_calls()
    {
        EXPECT_CALL(mock_egl, eglBindWaylandDisplayWL(_, _)).Times(0);
        EXPECT_CALL(mock_egl, eglQueryWaylandBufferWL(_, _, _, _)).Times(0);
        EXPECT_CALL(mock_egl, eglUnbindWaylandDisplayWL(_, _)).Times(0);
    }

    void expect_legacy_import()
    {
        EXPECT_CALL(mock_egl, eglQueryWaylandBufferWL(_, resource, EGL_WIDTH, _))
            .WillOnce(DoAll(SetArgPointee<3>(4), Return(EGL_TRUE)));
        EXPECT_CALL(mock_egl, eglQueryWaylandBufferWL(_, resource, EGL_HEIGHT, _))
            .WillOnce(DoAll(SetArgPointee<3>(4), Return(EGL_TRUE)));
        EXPECT_CALL(mock_egl, eglQueryWaylandBufferWL(_, resource, EGL_WAYLAND_Y_INVERTED_WL, _))
            .WillOnce(DoAll(SetArgPointee<3>(EGL_TRUE), Return(EGL_TRUE)));
        EXPECT_CALL(mock_egl, eglQueryWaylandBufferWL(_, resource, EGL_TEXTURE_FORMAT, _))
            .WillOnce(DoAll(SetArgPointee<3>(EGL_TEXTURE_RGBA), Return(EGL_TRUE)));
    }

    void check_import_callbacks()
    {
        MockFunction<void()> consumed;
        MockFunction<void()> released;
        EXPECT_CALL(consumed, Call()).Times(1);
        EXPECT_CALL(released, Call()).Times(1);
        auto buffer = allocator->buffer_from_resource(resource, consumed.AsStdFunction(), released.AsStdFunction());
        ASSERT_NE(nullptr, buffer);
        EXPECT_THAT(buffer->size(), Eq(mir::geometry::Size{4, 4}));
        auto texture = renderer->as_texture(buffer);
        EXPECT_NE(nullptr, renderer->as_texture(buffer));
        ASSERT_NE(nullptr, texture);
        texture->bind();
        texture->bind();
        texture.reset();
        buffer.reset();
    }

    NiceMock<mtd::MockEGL> mock_egl;
    NiceMock<mtd::MockGL> mock_gl;
    std::string extensions =
        "EGL_KHR_image_base EGL_EXT_image_dma_buf_import EGL_EXT_image_dma_buf_import_modifiers";
    std::shared_ptr<mtd::ExplicitExecutor> const wayland_executor = std::make_shared<mtd::ExplicitExecutor>();
    std::unique_ptr<wl_display, decltype(&wl_display_destroy)> display{nullptr, wl_display_destroy};
    std::unique_ptr<wl_display, decltype(&wl_display_disconnect)> client_display{nullptr, wl_display_disconnect};
    wl_client* raw_client = nullptr;
    std::shared_ptr<TestClient> client;
    wl_resource* resource = nullptr;
    std::shared_ptr<mgc::EGLContextExecutor> egl_executor;
    std::shared_ptr<mg::DMABufEGLProvider> provider;
    std::unique_ptr<mg::GraphicBufferAllocator> allocator;
    std::unique_ptr<mg::GLRenderingProvider> renderer;
};

TEST_P(EGLBufferAllocatorTest, missing_extension_rejects_non_shm_buffer_without_legacy_calls)
{
    expect_no_legacy_calls();
    ON_CALL(mock_egl, eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS))
        .WillByDefault(Return("EGL_WL_bind_wayland_display"));
    create_allocator();
    EXPECT_THROW(allocator->buffer_from_resource(resource, [] {}, [] {}), std::runtime_error);
    allocator->unbind_display(display.get());
}

TEST_P(EGLBufferAllocatorTest, failed_extension_query_disables_legacy_import)
{
    expect_no_legacy_calls();
    create_allocator();
    ON_CALL(mock_egl, eglQueryString(_, EGL_EXTENSIONS)).WillByDefault(Return(nullptr));
    EXPECT_NO_THROW(allocator->bind_display(display.get(), wayland_executor));
    EXPECT_THROW(allocator->buffer_from_resource(resource, [] {}, [] {}), std::runtime_error);
    allocator->unbind_display(display.get());
}

TEST_P(EGLBufferAllocatorTest, missing_extension_preserves_shm_import_and_callbacks)
{
    expect_no_legacy_calls();
    create_allocator();
    auto source = std::dynamic_pointer_cast<mir::renderer::software::RWMappableBuffer>(
        allocator->alloc_software_buffer({4, 4}, mir_pixel_format_argb_8888));
    ASSERT_NE(nullptr, source);
    MockFunction<void()> consumed;
    MockFunction<void()> released;
    EXPECT_CALL(consumed, Call()).Times(1);
    EXPECT_CALL(released, Call()).Times(1);
    auto buffer = allocator->buffer_from_shm(source, consumed.AsStdFunction(), released.AsStdFunction());
    ASSERT_NE(nullptr, buffer);
    EXPECT_NE(nullptr, renderer->as_texture(buffer));
    auto mappable = std::dynamic_pointer_cast<mir::renderer::software::RWMappableBuffer>(buffer);
    ASSERT_NE(nullptr, mappable);
    mappable->map_readable();
    mappable->map_readable();
    mappable.reset();
    buffer.reset();
    allocator->unbind_display(display.get());
}

TEST_P(EGLBufferAllocatorTest, missing_extension_preserves_dmabuf_import_and_callbacks)
{
    expect_no_legacy_calls();
    create_allocator(true);
    ASSERT_NO_FATAL_FAILURE(create_dmabuf());
    EXPECT_CALL(mock_egl, eglCreateImageKHR(_, _, EGL_LINUX_DMA_BUF_EXT, _, _)).Times(1);
    check_import_callbacks();
    allocator->unbind_display(display.get());
}

TEST_P(EGLBufferAllocatorTest, missing_extension_with_dmabuf_rejects_unrecognised_buffer)
{
    expect_no_legacy_calls();
    create_allocator(true);
    EXPECT_THROW(allocator->buffer_from_resource(resource, [] {}, [] {}), std::runtime_error);
    allocator->unbind_display(display.get());
}

TEST_P(EGLBufferAllocatorTest, legacy_import_works_without_dmabuf)
{
    extensions += " EGL_WL_bind_wayland_display";
    EXPECT_CALL(mock_egl, eglBindWaylandDisplayWL(_, display.get())).WillOnce(Return(EGL_TRUE));
    create_allocator();
    expect_legacy_import();
    check_import_callbacks();
    EXPECT_CALL(mock_egl, eglUnbindWaylandDisplayWL(_, display.get())).WillOnce(Return(EGL_TRUE));
    allocator->unbind_display(display.get());
    allocator->unbind_display(display.get());
    EXPECT_THROW(allocator->buffer_from_resource(resource, [] {}, [] {}), std::runtime_error);
}

TEST_P(EGLBufferAllocatorTest, unrecognised_dmabuf_falls_back_to_legacy_import_with_callbacks)
{
    extensions += " EGL_WL_bind_wayland_display";
    EXPECT_CALL(mock_egl, eglBindWaylandDisplayWL(_, display.get())).WillOnce(Return(EGL_TRUE));
    create_allocator(true);
    expect_legacy_import();
    check_import_callbacks();
    EXPECT_CALL(mock_egl, eglUnbindWaylandDisplayWL(_, display.get())).WillOnce(Return(EGL_TRUE));
    allocator->unbind_display(display.get());
}

TEST_P(EGLBufferAllocatorTest, dmabuf_import_is_preferred_over_legacy_import)
{
    extensions += " EGL_WL_bind_wayland_display";
    EXPECT_CALL(mock_egl, eglBindWaylandDisplayWL(_, display.get())).WillOnce(Return(EGL_TRUE));
    create_allocator(true);
    ASSERT_NO_FATAL_FAILURE(create_dmabuf());
    EXPECT_CALL(mock_egl, eglQueryWaylandBufferWL(_, _, _, _)).Times(0);
    check_import_callbacks();
    EXPECT_CALL(mock_egl, eglUnbindWaylandDisplayWL(_, display.get())).WillOnce(Return(EGL_TRUE));
    allocator->unbind_display(display.get());
}

TEST_P(EGLBufferAllocatorTest, failed_binding_disables_legacy_import_but_preserves_dmabuf)
{
    extensions += " EGL_WL_bind_wayland_display";
    EXPECT_CALL(mock_egl, eglBindWaylandDisplayWL(_, display.get())).WillOnce(Return(EGL_FALSE));
    EXPECT_CALL(mock_egl, eglQueryWaylandBufferWL(_, _, _, _)).Times(0);
    EXPECT_CALL(mock_egl, eglUnbindWaylandDisplayWL(_, _)).Times(0);
    create_allocator(true);
    EXPECT_THROW(allocator->buffer_from_resource(resource, [] {}, [] {}), std::runtime_error);
    ASSERT_NO_FATAL_FAILURE(create_dmabuf());
    check_import_callbacks();
    allocator->unbind_display(display.get());
}

TEST_P(EGLBufferAllocatorTest, missing_entry_point_disables_legacy_import_but_preserves_dmabuf)
{
    extensions += " EGL_WL_bind_wayland_display";
    ON_CALL(mock_egl, eglGetProcAddress(StrEq("eglQueryWaylandBufferWL"))).WillByDefault(Return(nullptr));
    expect_no_legacy_calls();
    create_allocator(true);
    ASSERT_NO_FATAL_FAILURE(create_dmabuf());
    check_import_callbacks();
    allocator->unbind_display(display.get());
}

INSTANTIATE_TEST_SUITE_P(
    Platforms, EGLBufferAllocatorTest, allocator_types,
    [](TestParamInfo<Allocator> const& info)
    {
        return info.param == Allocator::generic ? "Generic" : "GBM";
    });
