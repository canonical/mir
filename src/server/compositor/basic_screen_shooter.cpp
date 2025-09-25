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
    OneShotBufferDisplayProvider() = default;

    class FB : public mg::CPUAddressableDisplayAllocator::MappableFB
    {
    public:
        FB(std::shared_ptr<mrs::WriteMappableBuffer> buffer)
            : buffer{std::move(buffer)}
        {
        }

        auto map_writeable() -> std::unique_ptr<mrs::Mapping<std::byte>> override
        {
            return buffer->map_writeable();
        }
        auto size() const -> geom::Size override
        {
            return buffer->size();
        }
        auto format() const -> MirPixelFormat override
        {
            return buffer->format();
        }
        auto stride() const -> geom::Stride override
        {
            return buffer->stride();
        }
    private:
        std::shared_ptr<mrs::WriteMappableBuffer> const buffer;
    };

    auto supported_formats() const -> std::vector<graphics::DRMFormat> override
    {
        if (!next_buffer)
        {
            BOOST_THROW_EXCEPTION((std::logic_error{"Attempted to query supported_formats before assigning a buffer"}));
        }
        return {mg::DRMFormat::from_mir_format(next_buffer->format())};
    }

    auto alloc_fb(mg::DRMFormat format) -> std::unique_ptr<MappableFB> override
    {
        if (format.as_mir_format().value_or(mir_pixel_format_invalid) != next_buffer->format())
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Mismatched pixel formats"}));
        }
        return std::make_unique<FB>(std::exchange(next_buffer, nullptr));
    }

    auto output_size() const -> geom::Size override
    {
        if (!next_buffer)
        {
            BOOST_THROW_EXCEPTION((std::logic_error{"Attempted to query buffer size before assigning a buffer"}));
        }
        return next_buffer->size();
    }

    void set_next_buffer(std::shared_ptr<mrs::WriteMappableBuffer> buffer)
    {
        if (next_buffer)
        {
            BOOST_THROW_EXCEPTION((std::logic_error{"Attempt to set next buffer with a buffer already pending"}));
        }
        next_buffer = std::move(buffer);
    }
private:
    std::shared_ptr<mrs::WriteMappableBuffer> next_buffer;
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
    std::shared_ptr<graphics::Cursor> const& cursor)
    : scene{scene},
      clock{clock},
      render_provider{std::move(render_provider)},
      renderer_factory{std::move(renderer_factory)},
      last_rendered_size{0, 0},
      output{std::make_shared<OneShotBufferDisplayProvider>()},
      config{config},
      output_filter{output_filter},
      cursor{cursor}
{
}

auto mc::BasicScreenShooter::Self::render(
    std::shared_ptr<mrs::WriteMappableBuffer> const& buffer,
    geom::Rectangle const& area,
    glm::mat2 const& transform,
    bool overlay_cursor) -> time::Timestamp
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

    auto& renderer = renderer_for_buffer(buffer);
    renderer.set_output_transform(transform);
    renderer.set_viewport(area);
    renderer.set_output_filter(output_filter->filter());
    /* We don't need the result of this `render` call, as we know it's
     * going into the buffer we just set
     */
    renderer.render(renderable_list);

    // Because we might be called on a different thread next time we need to
    // ensure the renderer doesn't keep the EGL context current
    renderer.suspend();
    return captured_time;
}

auto mc::BasicScreenShooter::Self::renderer_for_buffer(std::shared_ptr<mrs::WriteMappableBuffer> buffer)
    -> mr::Renderer&
{
    auto const buffer_size = buffer->size();
    if (buffer_size.height == geom::Height{0} || buffer_size.width == geom::Width{0})
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Attempt to capture to a zero-sized buffer"}));
    }
    output->set_next_buffer(std::move(buffer));
    if (buffer_size != last_rendered_size)
    {
        // We need to build a new Renderer, at the new size
        offscreen_sink = std::make_unique<OffscreenDisplaySink>(output, buffer_size);
        auto gl_surface = render_provider->surface_for_sink(*offscreen_sink, *config);
        current_renderer = renderer_factory->create_renderer_for(std::move(gl_surface), render_provider);
        last_rendered_size = buffer_size;
    }
    return *current_renderer;
}

auto mc::BasicScreenShooter::select_provider(
    std::span<std::shared_ptr<mg::GLRenderingProvider>> const& providers,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator)
    -> std::shared_ptr<mg::GLRenderingProvider>
{
    auto display_provider = std::make_shared<Self::OneShotBufferDisplayProvider>();
    OffscreenDisplaySink temp_db{display_provider, geom::Size{640, 480}};

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
    std::shared_ptr<graphics::Cursor> const& cursor)
    : self{std::make_shared<Self>(scene, clock, select_provider(providers, buffer_allocator), std::move(render_factory), config, output_filter, cursor)},
      executor{executor}
{
}

void mc::BasicScreenShooter::capture(
    std::shared_ptr<mrs::WriteMappableBuffer> const& buffer,
    geom::Rectangle const& area,
    glm::mat2 const& transform,
    bool overlay_cursor,
    std::function<void(std::optional<time::Timestamp>)>&& callback)
{
    // TODO: use an atomic to keep track of number of in-flight captures, and error if it's too many

    executor.spawn([weak_self=std::weak_ptr{self}, buffer, area, transform, overlay_cursor, callback=std::move(callback)]
        {
            if (auto const self = weak_self.lock())
            {
                try
                {
                    callback(self->render(buffer, area, transform, overlay_cursor));
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

            callback(std::nullopt);
        });
}

mc::CompositorID mc::BasicScreenShooter::id() const
{
    return self.get();
}
