/*
 * Copyright © Canonical Ltd.
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
 */

#include "miral/application_switcher.h"
#include "wayland_app.h"
#include "wayland_shm.h"
#include "wlr-foreign-toplevel-management-unstable-v1.h"
#include "mir/default_font.h"

#include <memory>
#include <mir/fd.h>
#include <mir/log.h>
#include <mir/geometry/displacement.h>
#include <mutex>
#include <sys/eventfd.h>
#include <poll.h>
#include <string>
#include <cstring>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <codecvt>
#include <filesystem>
#include <locale>
#include <glib.h>
#include <gio/gio.h>

#include "layer_shell_wayland_surface.h"
#include "miral/wayland_extensions.h"

namespace geom = mir::geometry;
namespace msh = mir::shell;

namespace
{
struct ToplevelInfo
{
    zwlr_foreign_toplevel_handle_v1* handle;
    std::string app_id;
    std::string window_title;
};

struct preferred_codecvt : std::codecvt_byname<wchar_t, char, std::mbstate_t>
{
    preferred_codecvt() : std::codecvt_byname<wchar_t, char, std::mbstate_t>("C") {}
    ~preferred_codecvt() override = default;

    static std::locale::id id;
};

std::locale::id preferred_codecvt::id;

enum class SelectorState
{
    Applications,
    Windows
};

struct ToplevelInfoPrinter
{
    ToplevelInfoPrinter()
    {
        if (FT_Init_FreeType(&lib))
            return;

        if (FT_New_Face(lib, mir::default_font().c_str(), 0, &face))
        {
            mir::log_error("Failed to load find: %s", mir::default_font().c_str());
            FT_Done_FreeType(lib);
            return;
        }

        FT_Set_Pixel_Sizes(face, 0, 20);
        initialized = true;
    }

    ~ToplevelInfoPrinter()
    {
        if (initialized)
        {
            FT_Done_Face(face);
            FT_Done_FreeType(lib);
        }
    }

    ToplevelInfoPrinter(ToplevelInfoPrinter const&) = delete;
    ToplevelInfoPrinter& operator=(ToplevelInfoPrinter const&) = delete;

    void print(std::shared_ptr<miral::tk::WaylandShmBuffer> const& buffer,
        std::vector<ToplevelInfo> const& info_list,
        SelectorState selector_state,
        std::optional<int> selected_window_index)
    {
        if (!initialized)
            return;

        auto const region_size = buffer->size();
        FT_Set_Pixel_Sizes(face, 20, 0);

        const auto [width, height, line_height, lines, selected_line_index] = compute_text_metrics(info_list, selector_state, selected_window_index);
        auto base_pos_y = 0.5 * (region_size.height - height);
        static constexpr int region_pixel_bytes = 4;
        auto const region_buffer = static_cast<unsigned char*>(buffer->data());

        for (size_t i = 0; i < lines.size(); i++)
        {
            auto const& line = lines[i];
            auto base_pos_x = 0.5 * (region_size.width - width);
            auto line_text = converter.from_bytes(line);
            Color color = selected_line_index == i ? yellow : white;

            for (auto const ch : line_text)
            {
                FT_Load_Glyph(face, FT_Get_Char_Index(face, ch), FT_LOAD_DEFAULT);
                auto glyph = face->glyph;
                FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);

                geom::Point pos{glyph->bitmap_left, -glyph->bitmap_top};
                pos = pos + geom::Displacement{base_pos_x, base_pos_y};

                blend_glyph_into_region(glyph, pos, region_size, region_buffer, region_pixel_bytes, color);

                base_pos_x = base_pos_x + geom::DeltaX{glyph->advance.x >> 6};
            }
            base_pos_y = base_pos_y + line_height;
        }
    }

private:
    struct TextMetrics {
        geom::Width width{};
        geom::Height height{};
        geom::DeltaY line_height{};
        std::vector<std::string> lines;
        std::optional<int> selected_index;
    };

    struct Color {
        uint8_t r, g, b, a;
    };

    static constexpr Color white{255, 255, 255, 255};
    static constexpr Color yellow{255, 255, 0, 255};

    TextMetrics compute_text_metrics(
        std::vector<ToplevelInfo> const& info_list,
        SelectorState selector_state,
        std::optional<int> selected_window_index)
    {
        TextMetrics metrics;
        std::optional<ToplevelInfo> selected_top_level;
        if (selected_window_index)
            selected_top_level = info_list[*selected_window_index];

        // First, sort the information such that application ids are displayed in order alongside
        // their associated list of windows.
        std::vector<std::pair<std::string, std::vector<ToplevelInfo>>> list_of_app_id_to_window_mapping;
        for (auto const& info : info_list)
        {
            auto const& app_id = info.app_id;
            bool found = false;
            for (auto& mapping : list_of_app_id_to_window_mapping)
            {
                if (mapping.first == info.app_id)
                {
                    mapping.second.push_back(info);
                    found = true;
                    break;
                }
            }

            if (!found)
                list_of_app_id_to_window_mapping.push_back({app_id, {info}});
        }

        auto const add_text = [&](std::string const& text)
        {
            metrics.lines.push_back(text);
            auto const line_text = converter.from_bytes(text);
            geom::Width line_width{};

            for (auto const ch : line_text)
            {
                FT_Load_Glyph(face, FT_Get_Char_Index(face, ch), FT_LOAD_DEFAULT);
                auto const glyph = face->glyph;
                FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);

                line_width = line_width + geom::DeltaX{glyph->advance.x >> 6};
                metrics.line_height = std::max(metrics.line_height, geom::DeltaY{glyph->bitmap.rows * 1.5});
            }

            metrics.width = std::max(metrics.width, line_width);
            metrics.height = metrics.height + metrics.line_height;
        };

        // Using the sorted list, we then create the displayed list.
        for (const auto& [app_id, window_info_list] : list_of_app_id_to_window_mapping)
        {
            add_text(app_id);
            if (selected_top_level && selector_state == SelectorState::Applications)
            {
                if (selected_top_level->app_id == app_id)
                    metrics.selected_index = metrics.lines.size() - 1;
            }

            for (auto const& window_info : window_info_list)
            {
                add_text("    " + window_info.window_title);
                if (selected_top_level && selector_state == SelectorState::Windows)
                {
                    if (selected_top_level->handle == window_info.handle)
                        metrics.selected_index = metrics.lines.size() - 1;
                }
            }
        }
        return metrics;
    }

    static void blend_glyph_into_region(
        FT_GlyphSlot const& glyph,
        geom::Point const& pos,
        geom::Size const& region_size,
        unsigned char* region_buffer,
        int const region_pixel_bytes,
        Color const& color)
    {
        auto const glyph_size = geom::Size{glyph->bitmap.width, glyph->bitmap.rows};
        auto const region_top_left = geom::Point{-pos.x.as_value(), -pos.y.as_value()};
        auto const region_lower_right = region_top_left + as_displacement(region_size);

        auto const clipped_upper_left = geom::Point{
            std::max(geom::X{}, region_top_left.x),
            std::max(geom::Y{}, region_top_left.y)};
        auto const clipped_lower_right = geom::Point{
            std::min(geom::X{} + as_displacement(glyph_size).dx, region_lower_right.x),
            std::min(geom::Y{} + as_displacement(glyph_size).dy, region_lower_right.y)};

        unsigned char* glyph_buffer = glyph->bitmap.buffer;

        for (auto gy = clipped_upper_left.y; gy < clipped_lower_right.y; gy += geom::DeltaY{1})
        {
            auto ry = gy + (pos.y - geom::Y{});
            unsigned char* const glyph_row = glyph_buffer + gy.as_int() * glyph->bitmap.pitch;
            unsigned char* region_row =
                region_buffer + static_cast<long>(ry.as_int()) * static_cast<long>(region_size.width.as_int()) * region_pixel_bytes;

            for (auto gx = clipped_upper_left.x; gx < clipped_lower_right.x; gx += geom::DeltaX{1})
            {
                auto rx = gx + (pos.x - geom::X{});
                double const src_value = glyph_row[gx.as_int()] / 255.0;

                unsigned char* pixel = region_row + static_cast<long long>(rx.as_value()) * region_pixel_bytes;

                for (int c = 0; c < 3; c++)
                {
                    uint8_t const src_component =
                        (c == 0 ? color.b :   // blue goes in byte 0
                         c == 1 ? color.g :   // green in byte 1
                                  color.r);   // red in byte 2

                    pixel[c] = static_cast<uint8_t>(
                        src_component * src_value + pixel[c] * (1.0 - src_value));
                }
                pixel[3] = 255; // alpha
            }
        }
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    std::wstring_convert<preferred_codecvt> converter;
#pragma GCC diagnostic pop

    bool initialized = false;
    FT_Library lib = nullptr;
    FT_Face face = nullptr;
};

class WaylandApp : public miral::tk::WaylandApp
{
public:
    void init(wl_display* display)
    {
        wayland_init(display);
        if (!layer_shell())
            mir::fatal_error("The %s interface is required for the application switcher to run. "
                "Please enable it using miral::WaylandExtensions.", miral::WaylandExtensions::zwlr_layer_shell_v1);

        if (!toplevel_manager)
            mir::fatal_error("The %s interface is required for the application switcher to run. "
                "Please enable it using miral::WaylandExtensions.", miral::WaylandExtensions::zwlr_foreign_toplevel_manager_v1);

        shm_ = std::make_shared<miral::tk::WaylandShmPool>(shm());
        miral::tk::LayerShellWaylandSurface::CreationParams surface_params{
            mir::geometry::Size(0, 0),
            ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
            ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
                | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
        };
        surface_ = std::make_shared<miral::tk::LayerShellWaylandSurface>(this, surface_params);
        surface_->commit();
        roundtrip();
    }

    void add(zwlr_foreign_toplevel_handle_v1* toplevel)
    {
        std::lock_guard lock{mutex};
        toplevels_in_focus_order.push_back(ToplevelInfo{toplevel, "<unset>", "<unset>"});
        if (!tentative_focus_index)
            tentative_focus_index = 0;
    }

    void app_id(zwlr_foreign_toplevel_handle_v1* toplevel, char const* app_id)
    {
        std::lock_guard lock{mutex};
        auto const it = std::ranges::find_if(toplevels_in_focus_order, [toplevel](auto const& element)
        {
            return element.handle == toplevel;
        });
        if (it != toplevels_in_focus_order.end())
        {
            it->app_id = app_id;
        }
    }

    void window_title(zwlr_foreign_toplevel_handle_v1* toplevel, char const* window_title)
    {
        std::lock_guard lock{mutex};
        auto const it = std::ranges::find_if(toplevels_in_focus_order, [toplevel](auto const& element)
        {
            return element.handle == toplevel;
        });
        if (it != toplevels_in_focus_order.end())
        {
            it->window_title = window_title;
        }
    }

    void focus(zwlr_foreign_toplevel_handle_v1* toplevel)
    {
        auto const it = std::ranges::find_if(toplevels_in_focus_order, [toplevel](auto const& element)
        {
            return element.handle == toplevel;
        });

        if (it != toplevels_in_focus_order.end())
        {
            auto const element = *it;
            toplevels_in_focus_order.erase(it);
            toplevels_in_focus_order.insert(toplevels_in_focus_order.begin(), element);
        }
        else
        {
            toplevels_in_focus_order.insert(toplevels_in_focus_order.begin(), ToplevelInfo{toplevel, "<unset>", "<unset>"});
        }
    }

    void remove(zwlr_foreign_toplevel_handle_v1* toplevel)
    {
        std::lock_guard lock{mutex};
        auto const it = std::ranges::find_if(toplevels_in_focus_order, [toplevel](auto const& element)
        {
            return element.handle == toplevel;
        });
        if (it != toplevels_in_focus_order.end())
        {
            toplevels_in_focus_order.erase(it);

            if (toplevels_in_focus_order.empty())
                tentative_focus_index = std::nullopt;
            else if (!tentative_focus_index || *tentative_focus_index == toplevels_in_focus_order.size())
                tentative_focus_index = toplevels_in_focus_order.size() - 1;
        }
    }

    void confirm()
    {
        std::lock_guard lock{mutex};
        if (!is_running)
            return;

        if (tentative_focus_index)
            zwlr_foreign_toplevel_handle_v1_activate(toplevels_in_focus_order[*tentative_focus_index].handle, seat());
        is_running = false;
        tentative_focus_index = std::nullopt;
        draw_internal();
    }

    void cancel()
    {
        std::lock_guard lock{mutex};
        is_running = false;
        tentative_focus_index = std::nullopt;
        draw_internal();
    }

    void next_app()
    {
        std::lock_guard lock{mutex};
        if (start_if_not_running(SelectorState::Applications))
            return;

        if (!tentative_focus_index)
            return;

        // Find the window with the next unique application id.
        do
        {
            (*tentative_focus_index)++;
            if (*tentative_focus_index == toplevels_in_focus_order.size())
                *tentative_focus_index = 0;
        } while (!is_first_toplevel_with_app_id(toplevels_in_focus_order[*tentative_focus_index]));

        selector_state = SelectorState::Applications;
        draw_internal();
    }

    void prev_app()
    {
        std::lock_guard lock{mutex};
        if (start_if_not_running(SelectorState::Applications))
            return;

        if (!tentative_focus_index)
            return;

        // Find the window with the next unique application id in reverse.
        do
        {
            if (*tentative_focus_index == 0)
                tentative_focus_index = toplevels_in_focus_order.size() - 1;
            else
                (*tentative_focus_index)--;
        } while (!is_first_toplevel_with_app_id(toplevels_in_focus_order[*tentative_focus_index]));

        selector_state = SelectorState::Applications;
        draw_internal();
    }

    void next_window()
    {
        std::lock_guard lock{mutex};
        if (start_if_not_running(SelectorState::Windows))
            return;

        if (!tentative_focus_index)
            return;

        auto const current_app_id = toplevels_in_focus_order[*tentative_focus_index].app_id;

        do
        {
            (*tentative_focus_index)++;
            if (*tentative_focus_index == toplevels_in_focus_order.size())
                *tentative_focus_index = 0;
        } while (toplevels_in_focus_order[*tentative_focus_index].app_id != current_app_id);

        selector_state = SelectorState::Windows;
        draw_internal();
    }

    void prev_window()
    {
        std::lock_guard lock{mutex};
        if (start_if_not_running(SelectorState::Windows))
            return;

        if (!tentative_focus_index)
            return;

        auto const current_app_id = toplevels_in_focus_order[*tentative_focus_index].app_id;

        do
        {
            if (*tentative_focus_index == 0)
                tentative_focus_index = toplevels_in_focus_order.size() - 1;
            else
                (*tentative_focus_index)--;
        } while (toplevels_in_focus_order[*tentative_focus_index].app_id != current_app_id);

        selector_state = SelectorState::Windows;
        draw_internal();
    }

    void draw()
    {
        std::lock_guard lock{mutex};
        draw_internal();
    }

protected:
    void new_global(
        wl_registry* registry,
        uint32_t id,
        char const* interface,
        uint32_t) override
    {
        if (std::string_view(interface) == "zwlr_foreign_toplevel_manager_v1")
        {
            toplevel_manager = miral::tk::WaylandObject{
                static_cast<zwlr_foreign_toplevel_manager_v1*>(
                    wl_registry_bind(registry, id, &zwlr_foreign_toplevel_manager_v1_interface, 2)),
                zwlr_foreign_toplevel_manager_v1_destroy
            };
            zwlr_foreign_toplevel_manager_v1_add_listener(*toplevel_manager, &toplevel_manager_listener, this);
        }
    }

private:
    bool is_first_toplevel_with_app_id(ToplevelInfo const& info) const
    {
        for (auto const& other : toplevels_in_focus_order)
        {
            if (other.handle == info.handle)
                return true;
            if (other.app_id == info.app_id)
                return false;
        }

        return true;
    }

    static void handle_toplevel(void* data, zwlr_foreign_toplevel_manager_v1*, zwlr_foreign_toplevel_handle_v1* toplevel)
    {
        auto const self = static_cast<WaylandApp*>(data);
        self->add(toplevel);
        zwlr_foreign_toplevel_handle_v1_add_listener(toplevel, &self->toplevel_handle_listener, self);
    }
    static void handle_finished(void* data, zwlr_foreign_toplevel_manager_v1*)
    {
        auto const self = static_cast<WaylandApp*>(data);
        (void)self;
    }
    zwlr_foreign_toplevel_manager_v1_listener toplevel_manager_listener = {
        handle_toplevel,
        handle_finished
    };

    static void handle_title(void* data, zwlr_foreign_toplevel_handle_v1* handle, char const* window_title)
    {
        auto const self = static_cast<WaylandApp*>(data);
        self->window_title(handle, window_title);
    }
    static void handle_output_enter(void*, zwlr_foreign_toplevel_handle_v1*, wl_output*) {}
    static void handle_output_leave(void*, zwlr_foreign_toplevel_handle_v1*, wl_output*) {}
    static void handle_toplevel_done(void*, zwlr_foreign_toplevel_handle_v1*) {}
    static void handle_app_id(void* data, zwlr_foreign_toplevel_handle_v1* handle, char const* app_id)
    {
        auto const self = static_cast<WaylandApp*>(data);
        self->app_id(handle, app_id);
    }

    static void handle_state(void* data, zwlr_foreign_toplevel_handle_v1* handle, wl_array* states)
    {
        auto const self = static_cast<WaylandApp*>(data);
        auto const* states_casted = static_cast<zwlr_foreign_toplevel_handle_v1_state*>(states->data);
        for (size_t i = 0; i < states->size / sizeof(zwlr_foreign_toplevel_handle_v1_state); i++)
        {
            if (states_casted[i] == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED)
            {
                self->focus(handle);
                break;
            }
        }
    }

    static void handle_closed(void* data, zwlr_foreign_toplevel_handle_v1* handle)
    {
        auto const self = static_cast<WaylandApp*>(data);
        self->remove(handle);
    }

    zwlr_foreign_toplevel_handle_v1_listener toplevel_handle_listener = {
        .title = handle_title,
        .app_id = handle_app_id,
        .output_enter = handle_output_enter,
        .output_leave = handle_output_leave,
        .state = handle_state,
        .done = handle_toplevel_done,
        .closed = handle_closed
    };

    /// Starts the application switcher if it isn't already running.
    ///
    /// \returns `true` if it started, otherwise `false`.
    bool start_if_not_running(SelectorState next_state)
    {
        if (is_running)
            return false;

        is_running = true;
        tentative_focus_index = toplevels_in_focus_order.empty() ? std::optional<size_t>() : 0;
        selector_state = next_state;
        draw_internal();
        return true;
    }

    void draw_internal()
    {
        if (!is_running)
        {
            surface_->attach_buffer(nullptr, 1);
            surface_->commit();
            wl_display_flush(display());
            return;
        }

        mir::geometry::Size const size = surface_->configured_size();

        auto const width = size.width.as_int();
        auto const height = size.height.as_int();
        auto const stride = 4 * width;
        auto const buffer = shm_->get_buffer(geom::Size(width, height), geom::Stride(stride));

        auto row = static_cast<char*>(buffer->data());
        for (int j = 0; j < height; j++)
        {
            uint8_t pattern[4];
            uint8_t constexpr bottom_color[] = { 0x11, 0x11, 0x11 };   // Dark gray
            uint8_t constexpr top_color[] =    { 0x33, 0x33, 0x33 };   // Cool gray
            for (auto i = 0; i != 3; ++i)
                pattern[i] = (j * bottom_color[i] + (height - j) * top_color[i]) / height;

            if (j < static_cast<int>(static_cast<float>(height) / 8.f) || j >= static_cast<int>(static_cast<float>(height) * (7.f / 8.f)))
                pattern[3] = 0x22;
            else
                pattern[3] = 0xff;


            auto const pixel = reinterpret_cast<uint32_t*>(row);
            for (int i = 0; i < width; i++)
            {
                auto old_alpha = pattern[3];
                if (i < static_cast<int>(static_cast<float>(width) / 8.f) || i >= static_cast<int>(static_cast<float>(width) * (7.f / 8.f)))
                    pattern[3] = 0x22;
                mempcpy(pixel + i, pattern, sizeof pixel[i]);
                pattern[3] = old_alpha;
            }

            row += stride;
        }

        printer.print(
            buffer,
            toplevels_in_focus_order,
            selector_state,
            tentative_focus_index);

        surface_->attach_buffer(buffer->use(), 1);
        surface_->commit();
        wl_display_flush(display());
    }

    std::mutex mutex;
    std::optional<miral::tk::WaylandObject<zwlr_foreign_toplevel_manager_v1>> toplevel_manager;
    std::shared_ptr<miral::tk::WaylandShmPool> shm_;
    std::shared_ptr<miral::tk::LayerShellWaylandSurface> surface_;
    ToplevelInfoPrinter printer;
    std::vector<ToplevelInfo> toplevels_in_focus_order;
    std::optional<size_t> tentative_focus_index;
    SelectorState selector_state = SelectorState::Applications;
    bool is_running = false;
};
}

class miral::ApplicationSwitcher::Self
{
public:
    Self() : shutdown_signal(eventfd(0, EFD_CLOEXEC))
    {
        if (shutdown_signal == mir::Fd::invalid)
            mir::log_error("Failed to create shutdown notifier");
    }

    void run_client(wl_display* display)
    {
        {
            std::lock_guard lock{mutex};
            app = std::make_shared<WaylandApp>();
        }
        app->init(display);

        enum FdIndices {
            display_fd = 0,
            shutdown,
            indices
        };

        pollfd fds[indices];
        fds[display_fd] = {wl_display_get_fd(display), POLLIN, 0};
        fds[shutdown] = {shutdown_signal, POLLIN, 0};

        while (!(fds[shutdown].revents & (POLLIN | POLLERR)))
        {
            while (wl_display_prepare_read(display) != 0)
            {
                if (wl_display_dispatch_pending(display) == -1)
                    mir::log_error("Failed to dispatch Wayland events");
            }

            app->draw();

            if (poll(fds, indices, -1) == -1)
                mir::log_error("Failed to wait for Wayland events");

            if (fds[display_fd].revents & (POLLIN | POLLERR))
            {
                if (wl_display_read_events(display))
                    mir::log_error("Failed to read Wayland events");
            }
            else
            {
                wl_display_cancel_read(display);
            }
        }

        std::lock_guard lock{mutex};
        app = nullptr;
    }

    void confirm()
    {
        std::lock_guard lock{mutex};
        if (app)
            app->confirm();
    }

    void cancel()
    {
        std::lock_guard lock{mutex};
        if (app)
            app->cancel();
    }

    void stop() const
    {
        if (eventfd_write(shutdown_signal, 1) == -1)
            mir::log_error("Failed to notify internal decoration client to shutdown");
    }

    void next_app()
    {
        std::lock_guard lock{mutex};
        if (app)
            app->next_app();
    }

    void prev_app()
    {
        std::lock_guard lock{mutex};
        if (app)
            app->prev_app();
    }

    void next_window()
    {
        std::lock_guard lock{mutex};
        if (app)
            app->next_window();
    }

    void prev_window()
    {
        std::lock_guard lock{mutex};
        if (app)
            app->prev_window();
    }

private:
    mir::Fd const shutdown_signal;
    std::mutex mutex;
    std::shared_ptr<WaylandApp> app;
};

miral::ApplicationSwitcher::ApplicationSwitcher()
    : self(std::make_shared<Self>())
{
}

miral::ApplicationSwitcher::~ApplicationSwitcher()
{
    self->stop();
}

miral::ApplicationSwitcher::ApplicationSwitcher(ApplicationSwitcher const&) = default;

miral::ApplicationSwitcher& miral::ApplicationSwitcher::operator=(ApplicationSwitcher const&) = default;

void miral::ApplicationSwitcher::operator()(wl_display* display)
{
    self->run_client(display);
}

void miral::ApplicationSwitcher::operator()(std::weak_ptr<mir::scene::Session> const&) const
{
}

void miral::ApplicationSwitcher::next_app() const
{
    self->next_app();
}

void miral::ApplicationSwitcher::prev_app() const
{
    self->prev_app();
}

void miral::ApplicationSwitcher::next_window() const
{
    self->next_window();
}

void miral::ApplicationSwitcher::prev_window() const
{
    self->prev_window();
}

void miral::ApplicationSwitcher::confirm() const
{
    self->confirm();
}

void miral::ApplicationSwitcher::cancel() const
{
    self->cancel();
}

void miral::ApplicationSwitcher::stop() const
{
    self->stop();
}
