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

#include <miral/application_switcher.h>
#include "wayland_app.h"
#include "wayland_shm.h"
#include "wlr-foreign-toplevel-management-unstable-v1.h"
#include <mir/default_font.h>

#include <memory>
#include <mir/fd.h>
#include <mir/log.h>
#include <mir/geometry/displacement.h>
#include <mutex>
#include <queue>
#include <unordered_set>
#include <sys/eventfd.h>
#include <poll.h>
#include <string>
#include <cstring>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <codecvt>
#include <locale>

#include "layer_shell_wayland_surface.h"
#include <miral/append_keyboard_event_filter.h>
#include <miral/internal_client.h>
#include <miral/wayland_extensions.h>

namespace geom = mir::geometry;

namespace
{
struct ToplevelInfo
{
    zwlr_foreign_toplevel_handle_v1* handle;
    std::string app_id;
    std::string window_title;
    /// True until the first app_id or title event is received from the compositor.
    /// Ghost entries (done fired before any identity) are tracked but excluded from
    /// display and navigation — they are a Mir bug: https://github.com/canonical/mir/issues/4944
    bool ghost = true;
};

struct preferred_codecvt : std::codecvt_byname<wchar_t, char, std::mbstate_t>
{
    preferred_codecvt() : std::codecvt_byname<wchar_t, char, std::mbstate_t>("") {}
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
            mir::log_error("ApplicationSwitcher: Failed to load font: %s", mir::default_font().c_str());
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
        std::optional<size_t> selected_window_index)
    {
        if (!initialized)
            return;

        auto const region_size = buffer->size();
        FT_Set_Pixel_Sizes(face, 20, 0);

        auto const& [width, height, line_height, lines, is_app_header, selected_line_index] = compute_text_metrics(info_list, selector_state, selected_window_index);
        auto base_pos_y = 0.5 * (region_size.height - height);
        static constexpr int region_pixel_bytes = 4;
        auto const region_buffer = static_cast<unsigned char*>(buffer->data());

        for (size_t i = 0; i < lines.size(); i++)
        {
            auto const& line = lines[i];
            auto base_pos_x = 0.5 * (region_size.width - width);
            std::wstring const line_text = [&]
            {
                try
                {
                    return converter.from_bytes(line);
                }
                catch (std::exception const&)
                {
                    std::wstring ascii;
                    for (auto c : line) if (static_cast<unsigned char>(c) < 128) ascii += static_cast<wchar_t>(c);
                    return ascii;
                }
            }();
            Color color = selected_line_index == i ? yellow :
                is_app_header[i] ? grey   : white;

            for (auto const ch : line_text)
            {
                FT_Load_Glyph(face, FT_Get_Char_Index(face, static_cast<FT_ULong>(ch)), FT_LOAD_DEFAULT);
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
        std::vector<bool> is_app_header;
        std::optional<int> selected_index;
    };

    struct Color {
        uint8_t r, g, b, a;
    };

    static constexpr Color white{255, 255, 255, 255};
    static constexpr Color yellow{255, 255, 0, 255};
    static constexpr Color grey{128, 128, 128, 255};

    TextMetrics compute_text_metrics(
        std::vector<ToplevelInfo> const& info_list,
        SelectorState selector_state,
        std::optional<size_t> selected_window_index)
    {
        TextMetrics metrics;
        std::optional<ToplevelInfo> selected_top_level;
        if (selected_window_index)
            selected_top_level = info_list[*selected_window_index];

        // First, sort the information such that application ids are displayed in order alongside
        // their associated list of windows. Ghost entries (no app_id or title) are excluded.
        std::vector<std::pair<std::string, std::vector<ToplevelInfo>>> list_of_app_id_to_window_mapping;
        for (auto const& info : info_list)
        {
            if (info.ghost)
                continue;
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

        auto const add_text = [&](std::string const& text, bool is_header)
        {
            metrics.lines.push_back(text);
            metrics.is_app_header.push_back(is_header);
            std::wstring const line_text = [&]
            {
                try
                {
                    return converter.from_bytes(text);
                }
                catch (std::exception const&)
                {
                    std::wstring ascii;
                    for (auto c : text) if (static_cast<unsigned char>(c) < 128) ascii += static_cast<wchar_t>(c);
                    return ascii;
                }
            }();
            geom::Width line_width{};

            for (auto const ch : line_text)
            {
                FT_Load_Glyph(face, FT_Get_Char_Index(face, static_cast<FT_ULong>(ch)), FT_LOAD_DEFAULT);
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
            add_text(app_id, true);
            // App-id rows are never selected — the focus always sits on a window title.

            // In Applications mode the first window title of the selected app is highlighted.
            bool const highlight_first_window =
                selector_state == SelectorState::Applications
                && selected_top_level
                && selected_top_level->app_id == app_id;
            bool first_window = true;

            for (auto const& window_info : window_info_list)
            {
                add_text("    " + window_info.window_title, /*is_header=*/false);
                if (highlight_first_window && first_window)
                {
                    metrics.selected_index = static_cast<int>(metrics.lines.size()) - 1;
                    first_window = false;
                }

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

    ~WaylandApp() override
    {
        for (auto const& toplevel : toplevels_in_focus_order)
        {
            if (toplevel.handle)
            {
                zwlr_foreign_toplevel_handle_v1_destroy(toplevel.handle);
            }
        }
        toplevels_in_focus_order.clear();
        for (auto* handle : ghost_handles)
            zwlr_foreign_toplevel_handle_v1_destroy(handle);
        ghost_handles.clear();
    }

    void add(zwlr_foreign_toplevel_handle_v1* toplevel)
    {
        toplevels_in_focus_order.push_back(ToplevelInfo{toplevel, "<unset>", "<unset>"});
        if (!tentative_focus_index)
            tentative_focus_index = 0;
    }

    void focus(zwlr_foreign_toplevel_handle_v1* toplevel)
    {
        auto const it = std::ranges::find_if(toplevels_in_focus_order, [toplevel](auto const& element)
        {
            return element.handle == toplevel;
        });

        if (it != toplevels_in_focus_order.end())
        {
            if (tentative_focus_index)
            {
                auto const focused_idx = static_cast<size_t>(std::distance(toplevels_in_focus_order.begin(), it));
                if (focused_idx > 0 && *tentative_focus_index < focused_idx)
                    (*tentative_focus_index)++;
                else if (focused_idx > 0 && *tentative_focus_index == focused_idx)
                    tentative_focus_index = 0;
            }

            auto const element = *it;
            toplevels_in_focus_order.erase(it);
            toplevels_in_focus_order.insert(toplevels_in_focus_order.begin(), element);
        }
        else
        {
            mir::log_info("ApplicationSwitcher: focus() called for unknown toplevel handle — ignoring");
        }
    }

    void confirm()
    {
        if (!is_running)
            return;

        if (tentative_focus_index)
        {
            auto const& info = toplevels_in_focus_order[*tentative_focus_index];
            if (!info.ghost)
                zwlr_foreign_toplevel_handle_v1_activate(info.handle, seat());
        }
        is_running = false;
        tentative_focus_index = std::nullopt;
        draw_internal();
    }

    void cancel()
    {
        is_running = false;
        tentative_focus_index = std::nullopt;
        draw_internal();
    }

    void next_app()
    {
        if (start_if_not_running(SelectorState::Applications))
            return;

        if (!tentative_focus_index)
            return;

        // Find the window with the next unique application id.
        // Guaranteed to terminate: if every entry shares the same app_id (or all are
        // ghosts) the index wraps all the way back to its starting position, which passes
        // is_first_toplevel_with_app_id for the original entry.
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
        if (start_if_not_running(SelectorState::Applications))
            return;

        if (!tentative_focus_index)
            return;

        // Find the window with the next unique application id in reverse.
        // Guaranteed to terminate for the same reason as next_app().
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

protected:
    void new_global(
        wl_registry* registry,
        uint32_t id,
        char const* interface,
        uint32_t) override
    {
        if (std::string_view(interface) == "zwlr_foreign_toplevel_manager_v1")
        {
            toplevel_manager = miral::tk::WaylandObject<zwlr_foreign_toplevel_manager_v1>{
                static_cast<zwlr_foreign_toplevel_manager_v1*>(
                    wl_registry_bind(registry, id, &zwlr_foreign_toplevel_manager_v1_interface, 2)),
                zwlr_foreign_toplevel_manager_v1_destroy
            };
            zwlr_foreign_toplevel_manager_v1_add_listener(*toplevel_manager, &toplevel_manager_listener, this);
        }
    }

private:
    /// Erase the entry pointed to by `it` from `toplevels_in_focus_order` and adjust
    /// `tentative_focus_index` accordingly.  Does NOT destroy the Wayland handle.
    void remove_from_list(std::vector<ToplevelInfo>::iterator it)
    {
        auto const erased_index = static_cast<size_t>(std::distance(toplevels_in_focus_order.begin(), it));
        toplevels_in_focus_order.erase(it);

        if (toplevels_in_focus_order.empty())
        {
            tentative_focus_index = std::nullopt;
        }
        else if (tentative_focus_index)
        {
            if (*tentative_focus_index > erased_index)
                (*tentative_focus_index)--;
            if (*tentative_focus_index >= toplevels_in_focus_order.size())
                tentative_focus_index = toplevels_in_focus_order.size() - 1;
        }
    }

    bool is_first_toplevel_with_app_id(ToplevelInfo const& info) const
    {
        if (info.ghost)
            return false;  // ghosts are never navigable

        for (auto const& other : toplevels_in_focus_order)
        {
            if (other.ghost)
                continue;  // skip ghosts when scanning for duplicates
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

    void app_id(zwlr_foreign_toplevel_handle_v1* toplevel, char const* app_id)
    {
        if (ghost_handles.count(toplevel))
            return;
        auto const it = std::ranges::find_if(toplevels_in_focus_order, [toplevel](auto const& element)
        {
            return element.handle == toplevel;
        });
        if (it != toplevels_in_focus_order.end())
        {
            it->app_id = app_id;
            it->ghost = false;
        }
        else
        {
            mir::log_info("ApplicationSwitcher: app_id() event for unknown handle %p (app_id='%s') — missed add?",
                static_cast<void*>(toplevel), app_id);
        }
    }

    void window_title(zwlr_foreign_toplevel_handle_v1* toplevel, char const* window_title)
    {
        if (ghost_handles.count(toplevel))
            return;
        auto const it = std::ranges::find_if(toplevels_in_focus_order, [toplevel](auto const& element)
        {
            return element.handle == toplevel;
        });
        if (it != toplevels_in_focus_order.end())
        {
            it->window_title = window_title;
        }
        else
        {
            mir::log_info("ApplicationSwitcher: window_title() event for unknown handle %p (title='%s') — missed add?",
                static_cast<void*>(toplevel), window_title);
        }
    }

    void state(zwlr_foreign_toplevel_handle_v1* handle, wl_array* states)
    {
        if (ghost_handles.count(handle))
            return;

        auto const* states_casted = static_cast<zwlr_foreign_toplevel_handle_v1_state*>(states->data);
        for (size_t i = 0; i < states->size / sizeof(zwlr_foreign_toplevel_handle_v1_state); i++)
        {
            if (states_casted[i] == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED)
            {
                focus(handle);
                break;
            }
        }
    }

    void remove(zwlr_foreign_toplevel_handle_v1* toplevel)
    {
        if (ghost_handles.count(toplevel))
        {
            ghost_handles.erase(toplevel);
            zwlr_foreign_toplevel_handle_v1_destroy(toplevel);
            return;
        }

        auto const it = std::ranges::find_if(toplevels_in_focus_order, [toplevel](auto const& element)
        {
            return element.handle == toplevel;
        });
        if (it != toplevels_in_focus_order.end())
        {
            remove_from_list(it);
        }
        else
        {
            mir::log_info("ApplicationSwitcher: remove() closed event for unknown toplevel handle %p — already removed?",
                static_cast<void*>(toplevel));
        }
        zwlr_foreign_toplevel_handle_v1_destroy(toplevel);
    }

    zwlr_foreign_toplevel_handle_v1_listener toplevel_handle_listener = {
        .title = [](void* data, auto... args) { static_cast<WaylandApp*>(data)->window_title(args...); },
        .app_id = [](void* data, auto... args) { static_cast<WaylandApp*>(data)->app_id(args...); },
        .output_enter = [](void*, auto*, wl_output*) {},
        .output_leave = [](void*, auto*, wl_output*) {},
        .state = [](void* data, auto... args) { static_cast<WaylandApp*>(data)->state(args...); },
        .done = [](void* data, zwlr_foreign_toplevel_handle_v1* handle)
        {
            auto const self = static_cast<WaylandApp*>(data);

            if (self->ghost_handles.count(handle))
                return;

            auto const it = std::ranges::find_if(self->toplevels_in_focus_order, [handle](auto const& element)
            {
                return element.handle == handle;
            });

            if (it == self->toplevels_in_focus_order.end())
                return;

            if (it->ghost)
            {
                mir::log_info("ApplicationSwitcher: compositor sent done for toplevel with no app_id or title "
                    "(handle=%p) — likely a Mir bug. Hiding from switcher.", static_cast<void*>(handle));
                self->remove_from_list(it);
                self->ghost_handles.insert(handle);
            }
        },
        .closed = [](void* data, auto... args) { static_cast<WaylandApp*>(data)->remove(args...); },
    };

    /// Starts the application switcher if it isn't already running.
    ///
    /// \returns `true` if it started, otherwise `false`.
    bool start_if_not_running(SelectorState next_state)
    {
        if (is_running)
            return false;

        is_running = true;
        selector_state = next_state;

        // Find the first non-ghost entry to select initially.
        // Ghost entries (no app_id and no title received from compositor) are hidden from the UI.
        tentative_focus_index = std::nullopt;
        for (size_t i = 0; i < toplevels_in_focus_order.size(); i++)
        {
            if (!toplevels_in_focus_order[i].ghost)
            {
                tentative_focus_index = i;
                break;
            }
        }

        draw_internal();
        return true;
    }

    void draw_internal()
    {
        if (!is_running)
        {
            if (surface_visible)
            {
                surface_->attach_buffer(nullptr, 1);
                surface_->commit();
                wl_display_flush(display());
                surface_visible = false;
            }
            return;
        }

        mir::geometry::Size const size = surface_->configured_size();

        auto const width = size.width.as_int();
        auto const height = size.height.as_int();

        if (width <= 0 || height <= 0)
        {
            mir::log_info("ApplicationSwitcher: surface not yet configured (%dx%d) — deferring draw", width, height);
            return;
        }

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
        surface_visible = true;
        wl_display_flush(display());
    }

    std::optional<miral::tk::WaylandObject<zwlr_foreign_toplevel_manager_v1>> toplevel_manager;
    std::shared_ptr<miral::tk::WaylandShmPool> shm_;
    std::shared_ptr<miral::tk::LayerShellWaylandSurface> surface_;
    ToplevelInfoPrinter printer;
    std::vector<ToplevelInfo> toplevels_in_focus_order;
    /// Handles that were identified as Mir ghost toplevels (no app_id/title after done).
    /// All subsequent events for these handles are silently dropped.
    std::unordered_set<zwlr_foreign_toplevel_handle_v1*> ghost_handles;
    std::optional<size_t> tentative_focus_index;
    SelectorState selector_state = SelectorState::Applications;
    bool is_running = false;
    bool surface_visible = false;
};
}

class miral::ApplicationSwitcher::Self
{
    enum class Command { next_app, prev_app, next_window, prev_window, confirm, cancel };
public:
    Self() : shutdown_signal(eventfd(0, EFD_CLOEXEC)), command_signal(eventfd(0, EFD_CLOEXEC))
    {
        if (shutdown_signal == mir::Fd::invalid)
            mir::log_warning("ApplicationSwitcher: Failed to create shutdown notifier");
        if (command_signal == mir::Fd::invalid)
            mir::log_warning("ApplicationSwitcher: Failed to create command signal");
    }

    ~Self()
    {
        stop();
    }

    void run_client(wl_display* display)
    {
        app = std::make_shared<WaylandApp>();
        app->init(display);

        enum FdIndices {
            display_fd = 0,
            command_fd,
            shutdown_fd,
            indices
        };

        pollfd fds[indices];
        fds[display_fd] = {wl_display_get_fd(display), POLLIN, 0};
        fds[command_fd] = {command_signal, POLLIN, 0};
        fds[shutdown_fd] = {shutdown_signal, POLLIN, 0};

        try
        {
        while (!(fds[shutdown_fd].revents & (POLLIN | POLLERR)))
        {
            // Dispatch any events already in libwayland's queue (e.g. configure after
            // a commit, buffer releases). We do NOT redraw here — any draw needed was
            // already triggered by the command that caused the commit
            while (wl_display_prepare_read(display) != 0)
            {
                if (wl_display_dispatch_pending(display) == -1)
                {
                    mir::log_warning("ApplicationSwitcher: Failed to dispatch Wayland events (errno=%d)", errno);
                    break;
                }
            }

            // Flush any outgoing requests (ack_configure, or a command-triggered draw).
            if (wl_display_flush(display) == -1 && errno != EAGAIN)
                mir::log_warning("ApplicationSwitcher: wl_display_flush failed (errno=%d)", errno);

            if (poll(fds, indices, -1) == -1)
            {
                if (errno != EINTR)
                    mir::log_warning("ApplicationSwitcher: Failed to wait for Wayland events (errno=%d)", errno);
                wl_display_cancel_read(display);
                continue;
            }

            if (fds[display_fd].revents & (POLLIN | POLLERR))
            {
                if (wl_display_read_events(display))
                    mir::log_warning("ApplicationSwitcher: Failed to read Wayland events");
            }
            else
            {
                wl_display_cancel_read(display);
            }

            if (fds[command_fd].revents & (POLLIN | POLLERR))
            {
                eventfd_t val;
                eventfd_read(command_signal, &val);
                drain_commands();
            }
        }
        }
        catch (std::exception const& e)
        {
            mir::log_warning("ApplicationSwitcher: uncaught exception in Wayland thread: %s", e.what());
        }
        catch (...)
        {
            mir::log_warning("ApplicationSwitcher: unknown exception in Wayland thread");
        }

        app = nullptr;
    }

    void confirm()     { push_command(Command::confirm); }
    void cancel()      { push_command(Command::cancel); }
    void next_app()    { push_command(Command::next_app); }
    void prev_app()    { push_command(Command::prev_app); }
    void next_window() { push_command(Command::next_window); }
    void prev_window() { push_command(Command::prev_window); }

    void stop() const
    {
        if (shutdown_signal != mir::Fd::invalid && eventfd_write(shutdown_signal, 1) == -1)
            mir::log_warning("ApplicationSwitcher: Failed to send shutdown signal");
    }

private:
    void push_command(Command cmd)
    {
        {
            std::lock_guard lock{command_mutex};
            command_queue.push(cmd);
        }
        if (eventfd_write(command_signal, 1) == -1)
            mir::log_warning("ApplicationSwitcher: Failed to signal command to Wayland thread");
    }

    /// Called on the Wayland thread — safe to call app methods directly.
    void drain_commands()
    {
        while (true)
        {
            Command cmd;
            {
                std::lock_guard lock{command_mutex};
                if (command_queue.empty())
                    break;
                cmd = command_queue.front();
                command_queue.pop();
            }
            if (!app) continue;
            switch (cmd)
            {
            case Command::next_app:    app->next_app();    break;
            case Command::prev_app:    app->prev_app();    break;
            case Command::next_window: app->next_window(); break;
            case Command::prev_window: app->prev_window(); break;
            case Command::confirm:    app->confirm();     break;
            case Command::cancel:     app->cancel();      break;
            }
        }
    }

private:
    mir::Fd const shutdown_signal;
    mir::Fd const command_signal;
    std::mutex command_mutex;
    std::queue<Command> command_queue;

    // only accessed from the Wayland thread - no need for synchronization
    std::shared_ptr<WaylandApp> app;
};

miral::ApplicationSwitcher::ApplicationSwitcher()
    : self(std::make_shared<Self>())
{
}

miral::ApplicationSwitcher::~ApplicationSwitcher() = default;

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

class miral::BasicApplicationSwitcher::Self
{
public:
    explicit Self(KeybindConfiguration const& keybind_configuration)
        : startup_internal_client(switcher, switcher, [switcher=switcher] { switcher.stop(); }),
          keyboard_event_filter([this, keybind_configuration, is_running = false](MirKeyboardEvent const* key_event) mutable
         {
            auto const modifiers = toolkit::mir_keyboard_event_modifiers(key_event);
            if (toolkit::mir_keyboard_event_action(key_event) == mir_keyboard_action_down)
            {
                if (modifiers & keybind_configuration.primary_modifier)
                {
                    auto const scancode = toolkit::mir_keyboard_event_scan_code(key_event);
                    if (modifiers & keybind_configuration.reverse_modifier)
                    {
                        if (scancode == keybind_configuration.application_key)
                        {
                            switcher.prev_app();
                            is_running = true;
                            return true;
                        }
                        else if (scancode == keybind_configuration.window_key)
                        {
                            switcher.prev_window();
                            is_running = true;
                            return true;
                        }
                    }
                    else
                    {
                        if (scancode == keybind_configuration.application_key)
                        {
                            switcher.next_app();
                            is_running = true;
                            return true;
                        }
                        else if (scancode == keybind_configuration.window_key)
                        {
                            switcher.next_window();
                            is_running = true;
                            return true;
                        }
                    }
                }
            }
            else if (is_running && toolkit::mir_keyboard_event_action(key_event) == mir_keyboard_action_up)
            {
                if (!(modifiers & keybind_configuration.primary_modifier))
                {
                    switcher.confirm();
                    is_running = false;
                    return true;
                }
            }

            return false;
         })
    {
    }

    ApplicationSwitcher switcher;
    StartupInternalClient startup_internal_client;
    AppendKeyboardEventFilter keyboard_event_filter;
};

miral::BasicApplicationSwitcher::BasicApplicationSwitcher()
    : BasicApplicationSwitcher(KeybindConfiguration{}) {}

miral::BasicApplicationSwitcher::BasicApplicationSwitcher(KeybindConfiguration const& keybind_configuration)
    : self(std::make_shared<Self>(keybind_configuration))
{
}

miral::BasicApplicationSwitcher::~BasicApplicationSwitcher() = default;

void miral::BasicApplicationSwitcher::operator()(mir::Server& server) const
{
    self->keyboard_event_filter(server);
    self->startup_internal_client.operator()(server);
}
