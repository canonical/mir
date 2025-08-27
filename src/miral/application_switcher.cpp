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

#include "wayland_surface.h"

namespace geom = mir::geometry;

namespace
{
struct ToplevelInfo
{
    zwlr_foreign_toplevel_handle_v1* handle;
    std::optional<std::string> app_id;
};

struct preferred_codecvt : std::codecvt_byname<wchar_t, char, std::mbstate_t>
{
    preferred_codecvt() : std::codecvt_byname<wchar_t, char, std::mbstate_t>("C") {}
    ~preferred_codecvt() override = default;

    static std::locale::id id;
};

std::locale::id preferred_codecvt::id;

struct Printer
{
    Printer()
    {
        if (FT_Init_FreeType(&lib))
            return;

        if (FT_New_Face(lib, default_font().c_str(), 0, &face))
        {
            mir::log_error("Failed to load find: %s", default_font().c_str());
            FT_Done_FreeType(lib);
            return;
        }

        FT_Set_Pixel_Sizes(face, 0, 20);
        initialized = true;
    }

    ~Printer()
    {
        if (initialized)
        {
            FT_Done_Face(face);
            FT_Done_FreeType(lib);
        }
    }

    Printer(Printer const&) = delete;
    Printer& operator=(Printer const&) = delete;

    void print(std::shared_ptr<WaylandShmBuffer> const& buffer,
        std::vector<ToplevelInfo> const& info_list,
        std::optional<std::string> const& selected_app_id)
    {
        if (!initialized)
            return;

        auto const region_size = buffer->size();
        FT_Set_Pixel_Sizes(face, 20, 0);

        auto const metrics = compute_text_metrics(info_list);
        auto base_pos_y = 0.5 * (region_size.height - metrics.height);
        static constexpr int region_pixel_bytes = 4;
        auto const region_buffer = static_cast<unsigned char*>(buffer->data());

        for (auto const& line : metrics.lines)
        {
            auto base_pos_x = 0.5 * (region_size.width - metrics.width);
            auto line_text = converter.from_bytes(line);
            Color color = (selected_app_id && *selected_app_id == line) ? yellow : white;

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
            base_pos_y = base_pos_y + metrics.line_height;
        }
    }

private:
    struct TextMetrics {
        geom::Width width{};
        geom::Height height{};
        geom::DeltaY line_height{};
        std::vector<std::string> lines;
    };

    struct Color {
        uint8_t r, g, b, a;
    };

    static constexpr Color white{255, 255, 255, 255};
    static constexpr Color yellow{255, 255, 0, 255};

    // Font search logic should be kept in sync with src/server/shell/decoration/renderer.cpp
    static auto default_font() -> std::string
    {
        struct FontPath
        {
            char const* filename;
            std::vector<char const*> prefixes;
        };

        FontPath const font_paths[]{
            FontPath{"Ubuntu-B.ttf", {
                "ubuntu-font-family",   // Ubuntu < 18.04
                "ubuntu",               // Ubuntu >= 18.04/Arch
            }},
            FontPath{"FreeSansBold.ttf", {
                "freefont",             // Debian/Ubuntu
                "gnu-free",             // Fedora/Arch
            }},
            FontPath{"DejaVuSans-Bold.ttf", {
                "dejavu",               // Ubuntu (others?)
                "",                     // Arch
            }},
            FontPath{"LiberationSans-Bold.ttf", {
                "liberation-sans",      // Fedora
                "liberation-sans-fonts",// Fedora >= 42
                "liberation",           // Arch/Ubuntu
            }},
            FontPath{"OpenSans-Bold.ttf", {
                "open-sans",            // Fedora/Ubuntu
            }},
        };

        char const* const font_path_search_paths[]{
            "/usr/share/fonts/truetype",    // Ubuntu/Debian
            "/usr/share/fonts/TTF",         // Arch
            "/usr/share/fonts",             // Fedora/Arch
        };

        std::vector<std::string> usable_search_paths;
        for (auto const& path : font_path_search_paths)
        {
            if (std::filesystem::exists(path))
                usable_search_paths.emplace_back(path);
        }

        for (auto const& font : font_paths)
        {
            for (auto const& prefix : font.prefixes)
            {
                for (auto const& path : usable_search_paths)
                {
                    auto const full_font_path = path + '/' + prefix + '/' + font.filename;
                    if (std::filesystem::exists(full_font_path))
                        return full_font_path;
                }
            }
        }

        return "";
    }

    TextMetrics compute_text_metrics(std::vector<ToplevelInfo> const& info_list)
    {
        TextMetrics metrics;

        for (auto const& info : info_list)
        {
            auto app_id = info.app_id ? info.app_id.value() : "Unknown";
            if (std::ranges::find(metrics.lines, app_id) != metrics.lines.end())
                continue;

            metrics.lines.push_back(app_id);
            auto line_text = converter.from_bytes(app_id);
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
        }
        return metrics;
    }

    void blend_glyph_into_region(
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
            unsigned char* glyph_row = glyph_buffer + gy.as_int() * glyph->bitmap.pitch;
            unsigned char* region_row =
                region_buffer + static_cast<long>(ry.as_int()) * static_cast<long>(region_size.width.as_int()) * region_pixel_bytes;

            for (auto gx = clipped_upper_left.x; gx < clipped_lower_right.x; gx += geom::DeltaX{1})
            {
                auto rx = gx + (pos.x - geom::X{});
                double src_value = glyph_row[gx.as_int()] / 255.0;

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
    FT_Library lib;
    FT_Face face;
};
}

class miral::ApplicationSwitcher::Self : public WaylandApp
{
public:
    Self() : shutdown_signal(eventfd(0, EFD_CLOEXEC))
    {
        if (shutdown_signal == mir::Fd::invalid)
            mir::log_error("Failed to create shutdown notifier");
    }

    ~Self() override
    {
        if (toplevel_manager)
            zwlr_foreign_toplevel_manager_v1_destroy(toplevel_manager);
    }

    void init(wl_display* display)
    {
        wayland_init(display);
        shm_ = std::make_shared<WaylandShm>(shm());
        surface_ = std::make_shared<WaylandSurface>(this);
        // surface_->set_fullscreen(nullptr);
        surface_->commit();
        roundtrip();
    }

    void add(zwlr_foreign_toplevel_handle_v1* toplevel)
    {
        std::lock_guard lock{mutex};
        toplevels_in_focus_order.push_back(ToplevelInfo{toplevel, std::nullopt});
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

    /// Focus the provided toplevel.
    ///
    /// This entails bringing it to the front of the focus order.
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
            toplevels_in_focus_order.insert(toplevels_in_focus_order.begin(), ToplevelInfo{toplevel, std::nullopt});
        }

        std::size_t index = std::distance(std::begin(toplevels_in_focus_order), it);
        if (index == tentative_focus_index)
            tentative_focus_index = 0;
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

    void run_client(wl_display* display)
    {
        init(display);

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

            draw();

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
    }

    void confirm()
    {
        std::lock_guard lock{mutex};
        if (tentative_focus_index)
            zwlr_foreign_toplevel_handle_v1_activate(toplevels_in_focus_order[*tentative_focus_index].handle, seat());
        stop();
    }

    void stop() const
    {
        if (eventfd_write(shutdown_signal, 1) == -1)
            mir::log_error("Failed to notify internal decoration client to shutdown");
    }

    void next_app()
    {
        std::lock_guard lock{mutex};
        if (!tentative_focus_index)
            return;

        // Find the window with the next unique application id.
        auto const start_index = *tentative_focus_index;
        auto const app_id = toplevels_in_focus_order[*tentative_focus_index].app_id;
        do
        {
            (*tentative_focus_index)++;
            if (*tentative_focus_index == toplevels_in_focus_order.size())
                *tentative_focus_index = 0;

        } while (start_index != *tentative_focus_index &&
            toplevels_in_focus_order[*tentative_focus_index].app_id == app_id);
    }

    void prev_app()
    {
        std::lock_guard lock{mutex};
        if (!tentative_focus_index)
            return;

        // Find the window with the next unique application id in reverse.
        auto const start_index = *tentative_focus_index;
        auto const app_id = toplevels_in_focus_order[*tentative_focus_index].app_id;
        do
        {
            if (*tentative_focus_index == 0)
                tentative_focus_index = toplevels_in_focus_order.size() - 1;
            else
                (*tentative_focus_index)--;

        } while (start_index != *tentative_focus_index &&
            toplevels_in_focus_order[*tentative_focus_index].app_id == app_id);
    }

    void draw()
    {
        Size const size{400, 400};
        auto const width = size.width.as_int();
        auto const height = size.height.as_int();
        auto const stride = 4 * width;
        auto const buffer = shm_->get_buffer(geom::Size(width, height), geom::Stride(stride));

        uint8_t const bottom_colour[] = { 0x00, 0x00, 0x00 };   // Ubuntu orange
        uint8_t const top_colour[] =    { 0x00, 0x00, 0x00 };   // Cool grey

        char* row = static_cast<decltype(row)>(buffer->data());

        for (int j = 0; j < height; j++)
        {
            uint8_t pattern[4];

            for (auto i = 0; i != 3; ++i)
                pattern[i] = (j*bottom_colour[i] + (height-j)*top_colour[i])/height;
            pattern[3] = 0xff;

            uint32_t* pixel = (uint32_t*)row;
            for (int i = 0; i < width; i++)
                mempcpy(pixel + i, pattern, sizeof pixel[i]);

            row += stride;
        }

        {
            std::lock_guard lock{mutex};
            printer.print(
                buffer,
                toplevels_in_focus_order,
                tentative_focus_index ? toplevels_in_focus_order[*tentative_focus_index].app_id : std::nullopt);
        }

        surface_->attach_buffer(buffer->use(), 1);
        surface_->commit();
        wl_display_flush(display());
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
            toplevel_manager = static_cast<zwlr_foreign_toplevel_manager_v1*>(
                wl_registry_bind(registry, id, &zwlr_foreign_toplevel_manager_v1_interface, 2));
            zwlr_foreign_toplevel_manager_v1_add_listener(toplevel_manager, &toplevel_manager_listener, this);
        }
    }

private:
    static void handle_toplevel(void* data, zwlr_foreign_toplevel_manager_v1*, zwlr_foreign_toplevel_handle_v1* toplevel);
    static void handle_finished(void* data, zwlr_foreign_toplevel_manager_v1*)
    {
        auto const self = static_cast<Self*>(data);
        (void)self;
    }
    zwlr_foreign_toplevel_manager_v1_listener toplevel_manager_listener = {
        handle_toplevel,
        handle_finished
    };

    static void handle_title(void*, zwlr_foreign_toplevel_handle_v1*, char const*) {}
    static void handle_output_enter(void*, zwlr_foreign_toplevel_handle_v1*, wl_output*) {}
    static void handle_output_leave(void*, zwlr_foreign_toplevel_handle_v1*, wl_output*) {}
    static void handle_toplevel_done(void*, zwlr_foreign_toplevel_handle_v1*) {}
    static void handle_app_id(void* data, zwlr_foreign_toplevel_handle_v1* handle, char const* app_id)
    {
        auto const self = static_cast<Self*>(data);
        self->app_id(handle, app_id);
    }

    static void handle_state(void* data, zwlr_foreign_toplevel_handle_v1* handle, wl_array* states)
    {
        auto const self = static_cast<Self*>(data);
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
        auto const self = static_cast<Self*>(data);
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

    mir::Fd const shutdown_signal;
    std::mutex mutex;
    zwlr_foreign_toplevel_manager_v1* toplevel_manager = nullptr;
    std::vector<ToplevelInfo> toplevels_in_focus_order;
    std::optional<size_t> tentative_focus_index;
    std::shared_ptr<WaylandShm> shm_;
    std::shared_ptr<WaylandSurface> surface_;
    Printer printer;
};

void miral::ApplicationSwitcher::Self::handle_toplevel(void* data, zwlr_foreign_toplevel_manager_v1*, zwlr_foreign_toplevel_handle_v1* toplevel)
{
    auto const self = static_cast<Self*>(data);
    self->add(toplevel);
    zwlr_foreign_toplevel_handle_v1_add_listener(toplevel, &self->toplevel_handle_listener, self);
}

miral::ApplicationSwitcher::ApplicationSwitcher()
    : self(std::make_shared<Self>())
{
}

miral::ApplicationSwitcher& miral::ApplicationSwitcher::with_default_gui(bool with_gui)
{
    // TODO
    (void)with_gui;
    return *this;
}

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

void miral::ApplicationSwitcher::confirm() const
{
    self->confirm();
}

void miral::ApplicationSwitcher::stop() const
{
    self->stop();
}


