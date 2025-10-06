/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "decoration_provider.h"

#include "wallpaper_config.h"

#include "wayland_app.h"
#include "wayland_surface.h"
#include "wayland_shm.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <poll.h>
#include <sys/eventfd.h>
#include <locale>
#include <codecvt>
#include <map>
#include <string>
#include <cstring>

#include <boost/throw_exception.hpp>
#include <system_error>
#include <iostream>

using namespace mir::geometry;

namespace
{
struct BackgroundInfo;

struct preferred_codecvt : std::codecvt_byname<wchar_t, char, std::mbstate_t>
{
    preferred_codecvt() : std::codecvt_byname<wchar_t, char, std::mbstate_t>("C") {}
    ~preferred_codecvt() = default;
};

struct Printer
{
    Printer();
    ~Printer();
    Printer(Printer const&) = delete;
    Printer& operator=(Printer const&) = delete;

    void printhelp(BackgroundInfo const& region);

private:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    std::wstring_convert<preferred_codecvt> converter;
#pragma GCC diagnostic pop

    bool working = false;
    FT_Library lib;
    FT_Face face;
};

Printer::Printer()
{
    if (FT_Init_FreeType(&lib))
        return;

    if (FT_New_Face(lib, wallpaper::font_file().c_str(), 0, &face))
    {
        std::cerr << "WARNING: failed to load font: \"" <<  wallpaper::font_file() << "\"\n";
        FT_Done_FreeType(lib);
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, 10);
    working = true;
}

Printer::~Printer()
{
    if (working)
    {
        FT_Done_Face(face);
        FT_Done_FreeType(lib);
    }
}

struct BackgroundInfo : WaylandSurface
{
    BackgroundInfo(
        WaylandApp const* app,
        WaylandOutput const* output)
        : WaylandSurface{app},
          output{output},
          shm{app->shm()}
    {
        set_fullscreen(output->wl());
        commit();
        app->roundtrip();
    }

    void configured() override
    {
        needs_redraw = true;
    }

    ~BackgroundInfo()
    {
        app()->roundtrip();
    }

    /// returns the size (in pixels) the buffer should be
    auto buffer_size() const -> Size { return configured_size() * output->scale(); }

    // Screen description
    WaylandOutput const* const output;
    WaylandShm shm;

    // Content
    void* content_area = nullptr;

    bool needs_redraw{true};

private:
    BackgroundInfo(BackgroundInfo const&) = delete;
    BackgroundInfo& operator=(BackgroundInfo const&) = delete;
};

void Printer::printhelp(BackgroundInfo const& region)
{
    if (!working)
        return;

    auto const region_size = region.buffer_size();

    if (region_size.width <= Width{} || region_size.height <= Height{})
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
    Width help_width;
    Height help_height;
    DeltaY line_height;

    for (char const* rawline : helptext)
    {
        auto const line_text = converter.from_bytes(rawline);
        auto line_width = Width{};

        for (auto const& character : line_text)
        {
            FT_Load_Glyph(face, FT_Get_Char_Index(face, character), FT_LOAD_DEFAULT);
            auto const glyph = face->glyph;
            FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);

            line_width = line_width + DeltaX{glyph->advance.x >> 6};
            line_height = std::max(line_height, DeltaY{glyph->bitmap.rows * 1.5});
        }

        help_width = std::max(help_width, line_width);
        help_height = help_height + line_height;
    }

    auto base_pos_y = 0.5*(region_size.height - help_height);
    static auto const region_pixel_bytes = 4;
    unsigned char* const region_buffer = reinterpret_cast<char unsigned*>(region.content_area);

    for (auto const* rawline : helptext)
    {
        auto base_pos_x = 0.5*(region_size.width - help_width);
        auto const line_text = converter.from_bytes(rawline);

        for (auto const& character : line_text)
        {
            FT_Load_Glyph(face, FT_Get_Char_Index(face, character), FT_LOAD_DEFAULT);
            auto const glyph = face->glyph;
            FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);

            auto const glyph_size = Size{glyph->bitmap.width, glyph->bitmap.rows};
            auto const pos = Point{glyph->bitmap_left, -glyph->bitmap_top} + Displacement{base_pos_x, base_pos_y};
            auto const region_top_left_in_glyph_space = Point{} + (pos - Point{}) * -1; // literally just -pos
            auto const region_lower_right_in_glyph_space = region_top_left_in_glyph_space + as_displacement(region_size);
            auto const clipped_glyph_upper_left = Point{
                    std::max(X{}, region_top_left_in_glyph_space.x),
                    std::max(Y{}, region_top_left_in_glyph_space.y)};
            auto const clipped_glyph_lower_right = Point{
                    std::min(X{} + as_displacement(glyph_size).dx, region_lower_right_in_glyph_space.x),
                    std::min(Y{} + as_displacement(glyph_size).dy, region_lower_right_in_glyph_space.y)};

            unsigned char* const glyph_buffer = glyph->bitmap.buffer;
            auto glyph_pixel = Point{};
            auto region_pixel = Point{};
            for (
                glyph_pixel.y = clipped_glyph_upper_left.y;
                glyph_pixel.y < clipped_glyph_lower_right.y;
                glyph_pixel.y += DeltaY{1})
            {
                region_pixel.y = glyph_pixel.y + (pos.y - Y{});
                unsigned char* glyph_buffer_row = glyph_buffer + glyph_pixel.y.as_int() * glyph->bitmap.pitch;
                unsigned char* const region_buffer_row =
                    region_buffer + ((long)region_pixel.y.as_int() * (long)region_size.width.as_int() * region_pixel_bytes);
                for (
                    glyph_pixel.x = clipped_glyph_upper_left.x;
                    glyph_pixel.x < clipped_glyph_lower_right.x;
                    glyph_pixel.x += DeltaX{1})
                {
                    region_pixel.x = glyph_pixel.x + (pos.x - X{});
                    double const source_value = glyph_buffer_row[glyph_pixel.x.as_int()] / 255.0;
                    (void)source_value;
                    unsigned char* const region_buffer_offset =
                        region_buffer_row + ((long long)region_pixel.x.as_int() * region_pixel_bytes);
                    for (
                        unsigned char* region_ptr = region_buffer_offset;
                        region_ptr < region_buffer_offset + region_pixel_bytes;
                        region_ptr++)
                    {
                        *region_ptr = 0xff - (int)((0xff - *region_ptr) * (1 - source_value));
                    }
                }
            }
            base_pos_x = base_pos_x + DeltaX{glyph->advance.x >> 6};
        }
        base_pos_y = base_pos_y + line_height;
    }
}

struct DecorationProviderClient : WaylandApp
{
public:
    DecorationProviderClient(wl_display* display);
    ~DecorationProviderClient();

    // Called after events were dispatched and now the loop is idle
    void events_dispatched();

private:
    void draw_background(BackgroundInfo& ctx) const;

    void output_ready(WaylandOutput const*) override;
    void output_changed(WaylandOutput const* output) override;
    void output_gone(WaylandOutput const*) override;

    std::map<WaylandOutput const*, std::shared_ptr<BackgroundInfo>> outputs;
};

DecorationProviderClient::DecorationProviderClient(wl_display* display)
{
    wayland_init(display);
}

void DecorationProviderClient::events_dispatched()
{
    bool needs_flush = false;
    for (auto const& output : outputs)
    {
        if (output.second->needs_redraw)
        {
            draw_background(*output.second);
            output.second->needs_redraw = false;
            needs_flush = true;
        }
    }
    if (needs_flush)
    {
        wl_display_flush(display());
    }
}

void DecorationProviderClient::output_ready(WaylandOutput const* output)
{
    outputs.insert({output, std::make_shared<BackgroundInfo>(this, output)});
}

void DecorationProviderClient::output_changed(WaylandOutput const* output)
{
    auto const p = outputs.find(output);
    if (p != end(outputs))
    {
        p->second->needs_redraw = true;
    }
}

void DecorationProviderClient::output_gone(WaylandOutput const* output)
{
    outputs.erase(output);
}

void DecorationProviderClient::draw_background(BackgroundInfo& ctx) const
{
    auto const width = ctx.buffer_size().width.as_int();
    auto const height = ctx.buffer_size().height.as_int();

    if (width <= 0 || height <= 0)
        return;

    auto const stride = 4*width;

    auto const buffer = ctx.shm.get_buffer(ctx.buffer_size(), Stride{stride});
    ctx.content_area = buffer->data();

    uint8_t const bottom_colour[] = { 0x20, 0x54, 0xe9 };   // Ubuntu orange
    uint8_t const top_colour[] =    { 0x33, 0x33, 0x33 };   // Cool grey

    char* row = static_cast<decltype(row)>(ctx.content_area);

    for (int j = 0; j < height; j++)
    {
        uint8_t pattern[4];

        for (auto i = 0; i != 3; ++i)
            pattern[i] = (j*bottom_colour[i] + (height-j)*top_colour[i])/height;
        pattern[3] = 0xff;

        uint32_t* pixel = (uint32_t*)row;
        for (int i = 0; i < width; i++)
            memcpy(pixel + i, pattern, sizeof pixel[i]);

        row += stride;
    }

    static Printer printer;

    printer.printhelp(ctx);

    ctx.attach_buffer(buffer->use(), ctx.output->scale());
    ctx.commit();
}

DecorationProviderClient::~DecorationProviderClient()
{
}
}

using namespace mir::geometry;

DecorationProvider::DecorationProvider() :
    shutdown_signal{::eventfd(0, EFD_CLOEXEC)}
{
    if (shutdown_signal == mir::Fd::invalid)
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{errno, std::system_category(), "Failed to create shutdown notifier"}));
    }
}

DecorationProvider::~DecorationProvider() = default;

void DecorationProvider::stop()
{
    if (eventfd_write(shutdown_signal, 1) == -1)
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{
                errno,
                std::system_category(),
                "Failed to shutdown internal decoration client"}));
    }
}

void DecorationProvider::operator()(wl_display* display)
{
    DecorationProviderClient self(display);

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
            {
                BOOST_THROW_EXCEPTION((
                    std::system_error{errno, std::system_category(), "Failed to dispatch Wayland events"}));
            }
        }

        self.events_dispatched();

        if (poll(fds, indices, -1) == -1)
        {
            wl_display_cancel_read(display);
            BOOST_THROW_EXCEPTION((
                std::system_error{errno, std::system_category(), "Failed to wait for event"}));
        }

        if (fds[display_fd].revents & (POLLIN | POLLERR))
        {
            if (wl_display_read_events(display))
            {
                BOOST_THROW_EXCEPTION((
                    std::system_error{errno, std::system_category(), "Failed to read Wayland events"}));
            }
        }
        else
        {
            wl_display_cancel_read(display);
        }
    }
}

void DecorationProvider::operator()(std::weak_ptr<mir::scene::Session> const& session)
{
    std::lock_guard lock{mutex};
    this->weak_session = session;
}

auto DecorationProvider::session() const -> std::shared_ptr<mir::scene::Session>
{
    std::lock_guard lock{mutex};
    return weak_session.lock();
}

bool DecorationProvider::is_decoration(miral::Window const& window) const
{
    return window.application() == session();
}
