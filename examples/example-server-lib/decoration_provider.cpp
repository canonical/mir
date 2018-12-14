/*
 * Copyright © 2016-2018 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "decoration_provider.h"

#include "titlebar_config.h"

#include "wayland_helpers.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <locale>
#include <codecvt>
#include <map>
#include <string>
#include <cstring>
#include <sstream>

#include <iostream>
#include <thread>

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
    std::wstring_convert<preferred_codecvt> converter;

    bool working = false;
    FT_Library lib;
    FT_Face face;
};

Printer::Printer()
{
    if (FT_Init_FreeType(&lib))
        return;

    if (FT_New_Face(lib, titlebar::font_file().c_str(), 0, &face))
    {
        std::cerr << "WARNING: failed to load font: \"" <<  titlebar::font_file() << "\"\n";
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

struct BackgroundInfo
{
    BackgroundInfo(Output const& output) :
        output{output}
    {}
    
    ~BackgroundInfo()
    {
        if (buffer)
            wl_buffer_destroy(buffer);

        if (shell_surface)
            wl_shell_surface_destroy(shell_surface);

        if (surface)
            wl_surface_destroy(surface);
    }

    // Screen description
    Output const& output;

    // Content
    void* content_area = nullptr;
    wl_surface* surface = nullptr;
    wl_shell_surface* shell_surface = nullptr;
    wl_buffer* buffer = nullptr;
};

void Printer::printhelp(BackgroundInfo const& region)
{
    if (!working)
        return;

    static char const* const helptext[] =
        {
            "Welcome to miral-shell",
            "",
            "Keyboard shortcuts:",
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
            "  o To exit: Ctrl-Alt-BkSp",
        };

    int help_width = 0;
    unsigned int help_height = 0;
    unsigned int line_height = 0;

    for (auto const* rawline : helptext)
    {
        int line_width = 0;

        auto const line = converter.from_bytes(rawline);

        auto const fwidth = std::min(region.output.width / 60, 20);

        FT_Set_Pixel_Sizes(face, fwidth, 0);

        for (auto const& ch : line)
        {
            FT_Load_Glyph(face, FT_Get_Char_Index(face, ch), FT_LOAD_DEFAULT);
            auto const glyph = face->glyph;
            FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);

            line_width += glyph->advance.x >> 6;
            line_height = std::max(line_height, glyph->bitmap.rows + glyph->bitmap.rows/2);
        }

        if (help_width < line_width) help_width = line_width;
        help_height += line_height;
    }

    int base_y = (region.output.height - help_height)/2;
    auto* const region_address = reinterpret_cast<char unsigned*>(region.content_area);

    for (auto const* rawline : helptext)
    {
        int base_x = (region.output.width - help_width)/2;

        auto const line = converter.from_bytes(rawline);

        for (auto const& ch : line)
        {
            FT_Load_Glyph(face, FT_Get_Char_Index(face, ch), FT_LOAD_DEFAULT);
            auto const glyph = face->glyph;
            FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);

            auto const& bitmap = glyph->bitmap;
            auto const x = base_x + glyph->bitmap_left;

            if (static_cast<int>(x + bitmap.width) <= region.output.width)
            {
                unsigned char* src = bitmap.buffer;

                auto const y = base_y - glyph->bitmap_top;
                auto* dest = region_address + y * 4*region.output.width + 4 * x;

                for (auto row = 0u; row != bitmap.rows; ++row)
                {
                    for (auto col = 0u; col != 4 * bitmap.width; ++col)
                    {
                        dest[col] = (0xff*src[col / 4] + (dest[col] * (0xff - src[col / 4])))/0xff;
                    }

                    src += bitmap.pitch;
                    dest += 4*region.output.width;

                    if (dest > region_address + region.output.height * 4*region.output.width)
                        break;
                }
            }

            base_x += glyph->advance.x >> 6;
        }
        base_y += line_height;
    }
}

using Outputs = std::map<Output const*, BackgroundInfo>;

}

using namespace mir::geometry;

struct DecorationProvider::Self
{
public:
    Self();

    void init(wl_display* display);
    void teardown();

private:
    void draw_background(BackgroundInfo& ctx) const;
    void on_new_output(Output const*);
    void on_output_changed(Output const*);
    void on_output_gone(Output const*);

    wl_display* display = nullptr;
    Globals globals;
    Outputs outputs;
};

DecorationProvider::Self::Self() :
    globals{
        [this](Output const& output) { on_new_output(&output); },
        [this](Output const& output) { on_output_changed(&output); },
        [this](Output const& output) { on_output_gone(&output); }
    }
{
}

void DecorationProvider::Self::on_output_changed(Output const* output)
{
    auto const p = outputs.find(output);
    if (p != end(outputs))
        draw_background(p->second);
}

void DecorationProvider::Self::on_output_gone(Output const* output)
{
    outputs.erase(output);
}

void DecorationProvider::Self::on_new_output(Output const* output)
{
    draw_background(outputs.insert({output, BackgroundInfo{*output}}).first->second);
}

void DecorationProvider::Self::init(wl_display* display)
{
    this->display = display;
    globals.init(display);
}

void DecorationProvider::Self::draw_background(BackgroundInfo& ctx) const
{
    auto const width = ctx.output.width;
    auto const height = ctx.output.height;

    if (width <= 0 || height <= 0)
        return;

    auto const stride = 4*width;

    if (!ctx.surface)
    {
        ctx.surface = wl_compositor_create_surface(this->globals.compositor);
    }

    if (!ctx.shell_surface)
    {
        ctx.shell_surface = wl_shell_get_shell_surface(this->globals.shell, ctx.surface);
        wl_shell_surface_set_fullscreen(
            ctx.shell_surface,
            WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT,
            0,
            ctx.output.output);
    }

    if (ctx.buffer)
    {
        wl_buffer_destroy(ctx.buffer);
    }

    {
        auto const shm_pool = make_scoped(
            make_shm_pool(this->globals.shm, stride * height, &ctx.content_area),
            &wl_shm_pool_destroy);

        ctx.buffer = wl_shm_pool_create_buffer(
                shm_pool.get(),
                0,
                width, height, stride,
                WL_SHM_FORMAT_ARGB8888);
    }

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

    wl_surface_attach(ctx.surface, ctx.buffer, 0, 0);
    wl_surface_commit(ctx.surface);
    wl_display_roundtrip(display);
}

void DecorationProvider::Self::teardown()
{
    outputs.clear();
    globals.teardown();
}

DecorationProvider::DecorationProvider() :
    self{std::make_shared<Self>()}
{
}

DecorationProvider::~DecorationProvider() = default;

void DecorationProvider::stop()
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    running = false;
    running_cv.notify_one();
}


void DecorationProvider::operator()(struct wl_display* display)
{
    self->init(display);

    std::unique_lock<decltype(mutex)> lock{mutex};
    running = true;

    running_cv.wait(lock, [this] { return !running; });

    self->teardown();
    wl_display_roundtrip(display);
}

void DecorationProvider::operator()(std::weak_ptr<mir::scene::Session> const& session)
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    this->weak_session = session;
}

auto DecorationProvider::session() const -> std::shared_ptr<mir::scene::Session>
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    return weak_session.lock();
}

bool DecorationProvider::is_decoration(miral::Window const& window) const
{
    return window.application() == session();
}
