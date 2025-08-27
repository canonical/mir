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
};

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

        FT_Set_Pixel_Sizes(face, 0, 10);
        working = true;
    }

    ~Printer()
    {
        if (working)
        {
            FT_Done_Face(face);
            FT_Done_FreeType(lib);
        }
    }

    Printer(Printer const&) = delete;
    Printer& operator=(Printer const&) = delete;

    void printhelp(std::shared_ptr<WaylandShmBuffer> const& buffer)
    {
        if (!working)
            return;

        auto const region_size = buffer->size();

        if (region_size.width <= geom::Width{} || region_size.height <= geom::Height{})
            return;

        static char const* const helptext[] =
            {
                "Welcome to miral-shell",
                "",
                "Keyboard shortcuts:",
                "",
                "  o Terminal: Ctrl-Alt-T/Ctrl-Alt-Shift-T",
                "  o X Terminal: Ctrl-Alt-X (if X11 is enabled)",
                "",
                "  o Switch apps: Alt-Tab, tap or click on the corresponding window",
                "  o Next (previous) app window: Alt-` (Alt-Shift-`)",
                "",
                "  o Move window: Alt-leftmousebutton drag (three finger drag)",
                "  o Resize window: Alt-middle_button drag (three finger pinch)",
                "",
                "  o Maximize/restore current window (to display size). : Alt-F11",
                "  o Maximize/restore current window (to display height): Shift-F11",
                "  o Maximize/restore current window (to display width) : Ctrl-F11",
                "",
                "  o Switch workspace: Meta-Alt-[F1|F2|F3|F4]",
                "  o Switch workspace taking active window: Meta-Ctrl-[F1|F2|F3|F4]",
                "",
                "  o Invert colors: Ctrl-I",
                "",
                "  o To exit: Ctrl-Alt-BkSp",
            };

        auto const min_char_width = std::min({region_size.width.as_int()/60, region_size.height.as_int()/35, 20});
        FT_Set_Pixel_Sizes(face, min_char_width, 0);
        geom::Width help_width;
        geom::Height help_height;
        geom::DeltaY line_height;

        for (char const* rawline : helptext)
        {
            auto const line_text = converter.from_bytes(rawline);
            auto line_width = geom::Width{};

            for (auto const& character : line_text)
            {
                FT_Load_Glyph(face, FT_Get_Char_Index(face, character), FT_LOAD_DEFAULT);
                auto const glyph = face->glyph;
                FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);

                line_width = line_width + geom::DeltaX{glyph->advance.x >> 6};
                line_height = std::max(line_height, geom::DeltaY{glyph->bitmap.rows * 1.5});
            }

            help_width = std::max(help_width, line_width);
            help_height = help_height + line_height;
        }

        auto base_pos_y = 0.5*(region_size.height - help_height);
        static auto constexpr region_pixel_bytes = 4;
        unsigned char* const region_buffer = static_cast<char unsigned*>(buffer->data());

        for (auto const* rawline : helptext)
        {
            auto base_pos_x = 0.5*(region_size.width - help_width);
            auto const line_text = converter.from_bytes(rawline);

            for (auto const& character : line_text)
            {
                FT_Load_Glyph(face, FT_Get_Char_Index(face, character), FT_LOAD_DEFAULT);
                auto const glyph = face->glyph;
                FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);

                auto const glyph_size = geom::Size{glyph->bitmap.width, glyph->bitmap.rows};
                auto const pos = geom::Point{glyph->bitmap_left, -glyph->bitmap_top} + geom::Displacement{base_pos_x, base_pos_y};
                auto const region_top_left_in_glyph_space = geom::Point{} + (pos - geom::Point{}) * -1; // literally just -pos
                auto const region_lower_right_in_glyph_space = region_top_left_in_glyph_space + as_displacement(region_size);
                auto const clipped_glyph_upper_left = geom::Point{
                        std::max(geom::X{}, region_top_left_in_glyph_space.x),
                        std::max(geom::Y{}, region_top_left_in_glyph_space.y)};
                auto const clipped_glyph_lower_right = geom::Point{
                        std::min(geom::X{} + geom::generic::as_displacement(glyph_size).dx, region_lower_right_in_glyph_space.x),
                        std::min(geom::Y{} + geom::generic::as_displacement(glyph_size).dy, region_lower_right_in_glyph_space.y)};

                unsigned char* const glyph_buffer = glyph->bitmap.buffer;
                auto glyph_pixel = geom::Point{};
                auto region_pixel = geom::Point{};
                for (
                    glyph_pixel.y = clipped_glyph_upper_left.y;
                    glyph_pixel.y < clipped_glyph_lower_right.y;
                    glyph_pixel.y += geom::DeltaY{1})
                {
                    region_pixel.y = glyph_pixel.y + (pos.y - geom::Y{});
                    unsigned char* glyph_buffer_row = glyph_buffer + glyph_pixel.y.as_int() * glyph->bitmap.pitch;
                    unsigned char* const region_buffer_row =
                        region_buffer + (static_cast<long>(region_pixel.y.as_int()) * static_cast<long>(region_size.width.as_int()) * region_pixel_bytes);
                    for (
                        glyph_pixel.x = clipped_glyph_upper_left.x;
                        glyph_pixel.x < clipped_glyph_lower_right.x;
                        glyph_pixel.x += geom::DeltaX{1})
                    {
                        region_pixel.x = glyph_pixel.x + (pos.x - geom::X{});
                        double const source_value = glyph_buffer_row[glyph_pixel.x.as_int()] / 255.0;
                        (void)source_value;
                        unsigned char* const region_buffer_offset =
                            region_buffer_row + (static_cast<long long>(region_pixel.x.as_value()) * region_pixel_bytes);
                        for (
                            unsigned char* region_ptr = region_buffer_offset;
                            region_ptr < region_buffer_offset + region_pixel_bytes;
                            region_ptr++)
                        {
                            *region_ptr = 0xff - (int)((0xff - *region_ptr) * (1 - source_value));
                        }
                    }
                }
                base_pos_x = base_pos_x + geom::DeltaX{glyph->advance.x >> 6};
            }
            base_pos_y = base_pos_y + line_height;
        }
    }

private:
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


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    std::wstring_convert<preferred_codecvt> converter;
#pragma GCC diagnostic pop

    bool working = false;
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
        surface_->set_fullscreen(nullptr);
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
        Size const size{surface_->configured_size()};
        auto const width = size.width.as_int();
        auto const height = size.height.as_int();
        auto const stride = 4*width;
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

        printer.printhelp(buffer);
        surface_->add_frame_callback([this]
        {
            draw();
        });
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


