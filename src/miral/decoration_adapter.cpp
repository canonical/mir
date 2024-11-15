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

#include "decoration_adapter.h"

auto miral::Renderer::BufferStreams::create_buffer_stream() -> std::shared_ptr<mc::BufferStream>
{
    auto const stream = session->create_buffer_stream(mg::BufferProperties{
        geom::Size{1, 1},
        buffer_format,
        mg::BufferUsage::software});
    return stream;
}

miral::Renderer::BufferStreams::BufferStreams(std::shared_ptr<ms::Session> const& session)
    : session{session},
      titlebar{create_buffer_stream()},
      left_border{create_buffer_stream()},
      right_border{create_buffer_stream()},
      bottom_border{create_buffer_stream()}
{
}

miral::Renderer::BufferStreams::~BufferStreams()
{
    session->destroy_buffer_stream(titlebar);
    session->destroy_buffer_stream(left_border);
    session->destroy_buffer_stream(right_border);
    session->destroy_buffer_stream(bottom_border);
}

auto miral::Renderer::streams_to_spec(std::shared_ptr<WindowState> window_state)
    -> msh::SurfaceSpecification
{
    using StreamSpecification = mir::shell::StreamSpecification;

    msh::SurfaceSpecification spec;
    spec.streams = std::vector<StreamSpecification>{};

    auto const emplace = [&](std::shared_ptr<mc::BufferStream> stream, geom::Rectangle rect)
    {
        if (rect.size.width > geom::Width{} && rect.size.height > geom::Height{})
            spec.streams.value().emplace_back(StreamSpecification{stream, as_displacement(rect.top_left)});
    };

    emplace(buffer_streams->titlebar, window_state->titlebar_rect());

    return spec;
}

auto miral::Renderer::update_render_submit(std::shared_ptr<WindowState> window_state)
{
    update_state(*window_state);

    std::vector<std::pair<
        std::shared_ptr<mc::BufferStream>,
        std::optional<std::shared_ptr<mg::Buffer>>>> new_buffers;

    render_titlebar(titlebar_pixels.get(), titlebar_size);

    new_buffers.emplace_back(
        buffer_streams->titlebar,
        make_buffer(titlebar_pixels.get(), titlebar_size));

    float inv_scale = 1.0f; // 1.0f / window_state->scale();
    for (auto const& pair : new_buffers)
    {
        if (pair.second)
            pair.first->submit_buffer(
                pair.second.value(),
                pair.second.value()->size() * inv_scale,
                {{0, 0}, geom::SizeD{pair.second.value()->size()}});
    }
}

void miral::DecorationAdapter::update()
{
    auto window_spec = [this]
    {
        msh::SurfaceSpecification spec;

        window_surface->set_window_margins(
            as_delta(window_state->titlebar_height()), geom::DeltaX{}, geom::DeltaY{}, geom::DeltaX{});

        if (window_state->window_size().width.as_value())
            spec.width = window_state->window_size().width;
        if (window_state->window_size().height.as_value())
            spec.height = window_state->window_size().height;

        spec.input_shape = {window_state->titlebar_rect()};

        return spec;
    }();

    auto stream_spec = renderer->streams_to_spec(window_state);
    stream_spec.update_from(window_spec);

    if (!stream_spec.is_empty())
    {
        shell->modify_surface(session, decoration_surface, stream_spec);
    }

    renderer->update_render_submit(window_state);
}
