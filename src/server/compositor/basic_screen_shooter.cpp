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

#include "basic_screen_shooter.h"
#include "mir/graphics/cursor.h"
#include "mir/graphics/drm_formats.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/renderer/renderer.h"
#include "mir/renderer/gl/gl_surface.h"
#include "mir/compositor/scene_element.h"
#include "mir/compositor/scene.h"
#include "mir/log.h"
#include "mir/executor.h"
#include "mir/graphics/platform.h"
#include "mir/renderer/renderer_factory.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/graphics/display_sink.h"
#include "mir/graphics/output_filter.h"

namespace mc = mir::compositor;
namespace mr = mir::renderer;
namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;
namespace geom = mir::geometry;

class mc::BasicScreenShooter::Self::OneShotBufferDisplayProvider : public mg::CPUAddressableDisplayAllocator
{
public:
    OneShotBufferDisplayProvider(std::shared_ptr<mg::GraphicBufferAllocator> allocator, geom::Size const& initial_size) :
        allocator{allocator},
        buffer_size{initial_size},
        next_buffer{allocator->alloc_software_buffer(buffer_size, buffer_format)}
    {
    }

    class FB : public mg::CPUAddressableDisplayAllocator::MappableFB
    {
    public:
        FB(std::shared_ptr<mg::Buffer> buffer) :
            buffer{buffer},
            writable{mrs::as_write_mappable_buffer(buffer)}
        {
        }

        auto map_writeable() -> std::unique_ptr<mrs::Mapping<unsigned char>> override
        {
            return writable->map_writeable();
        }
        auto size() const -> geom::Size override
        {
            return buffer->size();
        }
        auto format() const -> MirPixelFormat override
        {
            return writable->format();
        }
        auto stride() const -> geom::Stride override
        {
            return writable->stride();
        }
        auto native_buffer_base() -> mg::NativeBufferBase* override
        {
            return buffer->native_buffer_base();
        }
        auto pixel_format() const -> MirPixelFormat override
        {
            return format();
        }
    private:
        std::shared_ptr<mg::Buffer> const buffer;
        std::shared_ptr<mrs::WriteMappableBuffer> const writable;
    };

    auto supported_formats() const -> std::vector<graphics::DRMFormat> override
    {
        if (!next_buffer)
        {
            BOOST_THROW_EXCEPTION((std::logic_error{"Attempted to query supported_formats before assigning a buffer"}));
        }
        return {mg::DRMFormat::from_mir_format(buffer_format)};
    }

    auto alloc_fb(mg::DRMFormat format) -> std::unique_ptr<MappableFB> override
    {
        if (format.as_mir_format().value_or(mir_pixel_format_invalid) != buffer_format)
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Mismatched pixel formats"}));
        }
        return std::make_unique<FB>(
            std::exchange(next_buffer, allocator->alloc_software_buffer(buffer_size, buffer_format)));
    }

    auto output_size() const -> geom::Size override
    {
        if (!next_buffer)
        {
            BOOST_THROW_EXCEPTION((std::logic_error{"Attempted to query buffer size before assigning a buffer"}));
        }
        return next_buffer->size();
    }

    bool output_size_changed(geom::Size new_size)
    {
        auto const size_changed = buffer_size != new_size;

        buffer_size = new_size;
        if(size_changed)
            next_buffer = allocator->alloc_software_buffer(buffer_size, buffer_format);

        return size_changed;
    }

private:
    std::shared_ptr<mg::GraphicBufferAllocator> const allocator;

    geom::Size buffer_size;
    MirPixelFormat buffer_format{mir_pixel_format_argb_8888};
    std::shared_ptr<mg::Buffer> next_buffer;
};

class OffscreenDisplaySink : public mg::DisplaySink
{
public:
    OffscreenDisplaySink(
        std::shared_ptr<mg::CPUAddressableDisplayAllocator> provider,
        geom::Size size)
        : provider {std::move(provider)},
          size{size}
    {
    }

    auto view_area() const -> mir::geometry::Rectangle override
    {
        return geom::Rectangle{{0, 0}, size};
    }

    bool overlay(std::vector<mg::DisplayElement> const& /*renderlist*/) override
    {
        return false;
    }

    void set_next_image(std::unique_ptr<mg::Framebuffer>) override
    {
    }

    auto transformation() const -> glm::mat2 override
    {
        return glm::mat2{};
    }

protected:
    auto maybe_create_allocator(mir::graphics::DisplayAllocator::Tag const& type_tag)
        -> mg::DisplayAllocator* override
    {
        if (dynamic_cast<mg::CPUAddressableDisplayAllocator::Tag const*>(&type_tag))
        {
            return provider.get();
        }
        return nullptr;
    }
private:
    std::shared_ptr<mg::CPUAddressableDisplayAllocator> const provider;
    geom::Size const size;
};

mc::BasicScreenShooter::Self::Self(
    std::shared_ptr<Scene> const& scene,
    std::shared_ptr<time::Clock> const& clock,
    std::shared_ptr<mg::GLRenderingProvider> render_provider,
    std::shared_ptr<mr::RendererFactory> renderer_factory,
    std::shared_ptr<mir::graphics::GLConfig> const& config,
    std::shared_ptr<graphics::OutputFilter> const& output_filter,
    std::shared_ptr<graphics::Cursor> const& cursor,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator) :
    scene{scene},
    clock{clock},
    render_provider{std::move(render_provider)},
    renderer_factory{std::move(renderer_factory)},
    display_provider{std::make_shared<OneShotBufferDisplayProvider>(buffer_allocator, geom::Size{200, 300})},
    config{config},
    output_filter{output_filter},
    cursor{cursor}
{
}

auto mc::BasicScreenShooter::Self::render(
    geom::Rectangle const& area,
    glm::mat2 const& transform,
    bool overlay_cursor) -> std::pair<time::Timestamp, std::shared_ptr<mg::Buffer>>
{
    std::lock_guard lock{mutex};

    auto scene_elements = scene->scene_elements_for(this);
    auto const captured_time = clock->now();
    mg::RenderableList renderable_list;
    renderable_list.reserve(scene_elements.size());
    for (auto const& element : scene_elements)
    {
        renderable_list.push_back(element->renderable());
    }

    if (overlay_cursor)
    {
        if (auto const cursor_renderable = cursor->renderable())
            renderable_list.push_back(cursor_renderable);
    }

    scene_elements.clear();

    auto& renderer = renderer_for_size(area.size);
    renderer.set_output_transform(transform);
    renderer.set_viewport(area);
    renderer.set_output_filter(output_filter->filter());
    /* We don't need the result of this `render` call, as we know it's
     * going into the buffer we just set
     */

    auto frame = renderer.render(renderable_list);

    // Because we might be called on a different thread next time we need to
    // ensure the renderer doesn't keep the EGL context current
    renderer.suspend();
    return {captured_time, std::move(frame)};
}

auto mc::BasicScreenShooter::Self::renderer_for_size(geom::Size const& size)
    -> mr::Renderer&
{
    if (size.height == geom::Height{0} || size.width == geom::Width{0})
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Attempt to capture to a zero-sized buffer"}));
    }
    if (display_provider->output_size_changed(size))
    {
        // We need to build a new Renderer, at the new size
        offscreen_sink = std::make_unique<OffscreenDisplaySink>(display_provider, size);
        auto gl_surface = render_provider->surface_for_sink(*offscreen_sink, *config);
        current_renderer = renderer_factory->create_renderer_for(std::move(gl_surface), render_provider);
    }

    return *current_renderer;
}

auto mc::BasicScreenShooter::select_provider(
    std::span<std::shared_ptr<mg::GLRenderingProvider>> const& providers,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator)
    -> std::shared_ptr<mg::GLRenderingProvider>
{
    auto const size = geom::Size{640, 480};
    auto display_provider = std::make_shared<Self::OneShotBufferDisplayProvider>(buffer_allocator, size);
    OffscreenDisplaySink temp_db{display_provider, size};

    std::pair<mg::probe::Result, std::shared_ptr<mg::GLRenderingProvider>> best_provider = std::make_pair(
        mg::probe::unsupported, nullptr);
    for (auto const& provider: providers)
    {
        /* TODO: There might be more sensible ways to select a provider
         * (one in use by at least one DisplaySink, the only one in use, the lowest-powered one,...)
         * That will be a job for a policy object, later.
         *
         * For now, just use the first that claims to work.
         */
        auto suitability = provider->suitability_for_display(temp_db);
        // We also need to make sure that the GLRenderingProvider can access client buffers...
        if (provider->suitability_for_allocator(buffer_allocator) > mg::probe::unsupported &&
            suitability > best_provider.first)
        {
            best_provider = std::make_pair(suitability, provider);
        }
    }
    if (best_provider.first == mg::probe::unsupported)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"No rendering provider claims to support a CPU addressable target"}));
    }

    return best_provider.second;
}

mc::BasicScreenShooter::BasicScreenShooter(
    std::shared_ptr<Scene> const& scene,
    std::shared_ptr<time::Clock> const& clock,
    Executor& executor,
    std::span<std::shared_ptr<mg::GLRenderingProvider>> const& providers,
    std::shared_ptr<mr::RendererFactory> render_factory,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<mir::graphics::GLConfig> const& config,
    std::shared_ptr<graphics::OutputFilter> const& output_filter,
    std::shared_ptr<graphics::Cursor> const& cursor) :
    self{std::make_shared<Self>(
        scene,
        clock,
        select_provider(providers, buffer_allocator),
        std::move(render_factory),
        config,
        output_filter,
        cursor,
        buffer_allocator)},
    executor{executor}
{
}

void mc::BasicScreenShooter::capture(
    geom::Rectangle const& area,
    glm::mat2 const& transform,
    bool overlay_cursor,
    std::function<void(std::optional<time::Timestamp>, std::shared_ptr<mg::Buffer>)>&& callback)
{
    // TODO: use an atomic to keep track of number of in-flight captures, and error if it's too many

    if (area.size.width == geom::Width{0} || area.size.height == geom::Height{0})
    {
        callback(std::nullopt, nullptr);
        return;
    }

    executor.spawn([weak_self=std::weak_ptr{self}, area, transform, overlay_cursor, callback=std::move(callback)]
        {
            if (auto const self = weak_self.lock())
            {
                try
                {
                    auto [time, frame] = self->render(area, transform, overlay_cursor);
                    callback(time, frame);
                    return;
                }
                catch (...)
                {
                    mir::log(
                        ::mir::logging::Severity::error,
                        "BasicScreenShooter",
                        std::current_exception(),
                        "failed to capture screen");
                }
            }

            callback(std::nullopt, nullptr);
        });
}

mc::CompositorID mc::BasicScreenShooter::id() const
{
    return self.get();
}
