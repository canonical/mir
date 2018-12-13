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
char const* const wallpaper_name = "wallpaper";

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

//    void printhelp(MirGraphicsRegion const& region);

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

//void Printer::printhelp(MirGraphicsRegion const& region)
//{
//    if (!working)
//        return;
//
//    static char const* const helptext[] =
//        {
//            "Welcome to miral-shell",
//            "",
//            "Keyboard shortcuts:",
//            "",
//            "  o Switch apps: Alt-Tab, tap or click on the corresponding window",
//            "  o Next (previous) app window: Alt-` (Alt-Shift-`)",
//            "",
//            "  o Move window: Alt-leftmousebutton drag (three finger drag)",
//            "  o Resize window: Alt-middle_button drag (three finger pinch)",
//            "",
//            "  o Maximize/restore current window (to display size). : Alt-F11",
//            "  o Maximize/restore current window (to display height): Shift-F11",
//            "  o Maximize/restore current window (to display width) : Ctrl-F11",
//            "",
//            "  o Switch workspace: Meta-Alt-[F1|F2|F3|F4]",
//            "  o Switch workspace taking active window: Meta-Ctrl-[F1|F2|F3|F4]",
//            "",
//            "  o To exit: Ctrl-Alt-BkSp",
//        };
//
//    int help_width = 0;
//    unsigned int help_height = 0;
//    unsigned int line_height = 0;
//
//    for (auto const* rawline : helptext)
//    {
//        int line_width = 0;
//
//        auto const line = converter.from_bytes(rawline);
//
//        auto const fwidth = std::min(region.width / 60, 20);
//
//        FT_Set_Pixel_Sizes(face, fwidth, 0);
//
//        for (auto const& ch : line)
//        {
//            FT_Load_Glyph(face, FT_Get_Char_Index(face, ch), FT_LOAD_DEFAULT);
//            auto const glyph = face->glyph;
//            FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);
//
//            line_width += glyph->advance.x >> 6;
//            line_height = std::max(line_height, glyph->bitmap.rows + glyph->bitmap.rows/2);
//        }
//
//        if (help_width < line_width) help_width = line_width;
//        help_height += line_height;
//    }
//
//    int base_y = (region.height - help_height)/2;
//    auto* const region_address = reinterpret_cast<char unsigned*>(region.vaddr);
//
//    for (auto const* rawline : helptext)
//    {
//        int base_x = (region.width - help_width)/2;
//
//        auto const line = converter.from_bytes(rawline);
//
//        for (auto const& ch : line)
//        {
//            FT_Load_Glyph(face, FT_Get_Char_Index(face, ch), FT_LOAD_DEFAULT);
//            auto const glyph = face->glyph;
//            FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);
//
//            auto const& bitmap = glyph->bitmap;
//            auto const x = base_x + glyph->bitmap_left;
//
//            if (static_cast<int>(x + bitmap.width) <= region.width)
//            {
//                unsigned char* src = bitmap.buffer;
//
//                auto const y = base_y - glyph->bitmap_top;
//                auto* dest = region_address + y * region.stride + 4 * x;
//
//                for (auto row = 0u; row != bitmap.rows; ++row)
//                {
//                    for (auto col = 0u; col != 4 * bitmap.width; ++col)
//                    {
//                        dest[col] = (0xff*src[col / 4] + (dest[col] * (0xff - src[col / 4])))/0xff;
//                    }
//
//                    src += bitmap.pitch;
//                    dest += region.stride;
//
//                    if (dest > region_address + region.height * region.stride)
//                        break;
//                }
//            }
//
//            base_x += glyph->advance.x >> 6;
//        }
//        base_y += line_height;
//    }
//}
//
//void render_background(MirBufferStream* buffer_stream, MirGraphicsRegion& graphics_region)
//{
//    {
//        uint8_t const bottom_colour[] = { 0x20, 0x54, 0xe9 };   // Ubuntu orange
//        uint8_t const top_colour[] =    { 0x33, 0x33, 0x33 };   // Cool grey
//
//        char* row = graphics_region.vaddr;
//
//        for (int j = 0; j < graphics_region.height; j++)
//        {
//            uint8_t pattern[4];
//
//            for (auto i = 0; i != 3; ++i)
//                pattern[i] = (j*bottom_colour[i] + (graphics_region.height-j)*top_colour[i])/graphics_region.height;
//            pattern[3] = 0xff;
//
//            uint32_t* pixel = (uint32_t*)row;
//            for (int i = 0; i < graphics_region.width; i++)
//                memcpy(pixel + i, pattern, sizeof pixel[i]);
//
//            row += graphics_region.stride;
//        }
//    }
//
    static Printer printer;
//    printer.printhelp(*&graphics_region);
//
//    mir_buffer_stream_swap_buffers_sync(buffer_stream);
//}

struct OutputInfo
{
    OutputInfo(int32_t x, int32_t y, int32_t width, int32_t height) :
        x{x}, y{y}, width{width}, height{height}
    {}

    // Screen description
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;

    // Content
    void* content_area = nullptr;
    struct wl_display* display;
    struct wl_surface* surface;
    struct wl_callback* new_frame_signal;
    wl_buffer* buffer = nullptr;
    bool waiting_for_buffer;
};

using Outputs = std::map<struct wl_output*, OutputInfo>;

void output_geometry(void *data,
                     struct wl_output *wl_output,
                     int32_t x,
                     int32_t y,
                     int32_t /*physical_width*/,
                     int32_t /*physical_height*/,
                     int32_t /*subpixel*/,
                     const char */*make*/,
                     const char */*model*/,
                     int32_t /*transform*/)
{
    Outputs* outputs = static_cast<decltype(outputs)>(data);

    auto const& output = outputs->find(wl_output);
    if (output != outputs->end())
    {
        output->second.x = x;
        output->second.y = y;
    }
    else
    {
        outputs->insert({wl_output, {x, y, 0, 0}});
    }
}

void output_mode(void *data,
                 struct wl_output *wl_output,
                 uint32_t flags,
                 int32_t width,
                 int32_t height,
                 int32_t /*refresh*/)
{
    if (!(WL_OUTPUT_MODE_CURRENT & flags))
        return;

    Outputs* outputs = static_cast<decltype(outputs)>(data);

    auto const& output = outputs->find(wl_output);
    if (output != outputs->end())
    {
        output->second.width = width;
        output->second.height = height;
    }
    else
    {
        outputs->insert({wl_output, {0, 0, width, height}});
    }
}

void output_done(void* data, struct wl_output* wl_output)
{
    (void)data;
    (void)wl_output;
}

void output_scale(void* data, struct wl_output* wl_output, int32_t factor)
{
    (void)data;
    (void)wl_output;
    (void)factor;
}

wl_output_listener const output_listener = {
    &output_geometry,
    &output_mode,
    &output_done,
    &output_scale,
};
}

using namespace mir::geometry;

struct DecorationProvider::Self
{
public:
    Self(miral::WindowManagerTools const& tools);

    void init(struct wl_display* display);

private:
    miral::WindowManagerTools tools;
    Globals globals;
    Outputs outputs;

//    mir::client::Connection connection;
//    struct Wallpaper { mir::client::Surface surface; mir::client::Window window; MirBufferStream* stream; };
//    std::vector<Wallpaper> wallpaper;
//
//    static void handle_event_for_background(MirWindow* window, MirEvent const* event, void* context_);
//    void handle_event_for_background(MirWindow* window, MirEvent const* ev);
};

DecorationProvider::Self::Self(miral::WindowManagerTools const& tools) :
    tools{tools}
{
}

void DecorationProvider::Self::init(struct wl_display* display)
{
    globals.init(display);
    wl_output_add_listener(globals.output, &output_listener, &outputs);
    wl_display_roundtrip(display);

    for (auto& o : outputs)
    {
        auto& ctx = o.second;

        struct wl_shm_pool* shm_pool = make_shm_pool(globals.shm, ctx.width * ctx.height * 4, &ctx.content_area);
        ctx.buffer = wl_shm_pool_create_buffer(shm_pool, 0, ctx.width, ctx.height, ctx.width*4, WL_SHM_FORMAT_ARGB8888);
        wl_shm_pool_destroy(shm_pool);

        ctx.display = display;
        ctx.surface = wl_compositor_create_surface(globals.compositor);

        auto const window = make_scoped(wl_shell_get_shell_surface(globals.shell, ctx.surface), &wl_shell_surface_destroy);
        wl_shell_surface_set_fullscreen(window.get(), WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT, 0, nullptr);

        uint8_t const bottom_colour[] = { 0x20, 0x54, 0xe9 };   // Ubuntu orange
        uint8_t const top_colour[] =    { 0x33, 0x33, 0x33 };   // Cool grey

        char* row = static_cast<decltype(row)>(ctx.content_area);

        for (int j = 0; j < ctx.height; j++)
        {
            uint8_t pattern[4];

            for (auto i = 0; i != 3; ++i)
                pattern[i] = (j*bottom_colour[i] + (ctx.height-j)*top_colour[i])/ctx.height;
            pattern[3] = 0xff;

            uint32_t* pixel = (uint32_t*)row;
            for (int i = 0; i < ctx.width; i++)
                memcpy(pixel + i, pattern, sizeof pixel[i]);

            row += 4*ctx.width;
        }

        wl_surface_attach(ctx.surface, ctx.buffer, 0, 0);
        wl_surface_commit(ctx.surface);
    }
}

DecorationProvider::DecorationProvider(miral::WindowManagerTools const& tools) :
    self{std::make_shared<Self>(tools)}
{
}

DecorationProvider::~DecorationProvider() = default;

void DecorationProvider::stop()
{
    running = false;
}


void DecorationProvider::operator()(struct wl_display* display)
{
    self->init(display);

    running = true;
    do
    {
        wl_display_dispatch(display);
    }
    while (running);
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

//void DecorationProvider::handle_event_for_background(MirWindow* window, MirEvent const* event, void* context)
//{
//    static_cast<DecorationProvider*>(context)->handle_event_for_background(window, event);
//}
//
//void DecorationProvider::handle_event_for_background(MirWindow* window, MirEvent const* ev)
//{
//    switch (mir_event_get_type(ev))
//    {
//    case mir_event_type_resize:
//    {
//        MirResizeEvent const* resize = mir_event_get_resize_event(ev);
//        int const new_width = mir_resize_event_get_width(resize);
//        int const new_height = mir_resize_event_get_height(resize);
//
//        enqueue_work([window, new_width, new_height, this]()
//            {
//                auto found = find_if(begin(wallpaper), end(wallpaper), [&](Wallpaper const& w) { return w.window == window;});
//                if (found == end(wallpaper)) return;
//
//                mir_buffer_stream_set_size(found->stream, new_width, new_height);
//                mir_render_surface_set_size(found->surface, new_width, new_height);
//
//                WindowSpec::for_changes(connection)
//                    .add_surface(found->surface, new_width, new_height, 0, 0)
//                    .apply_to(window);
//
//                MirGraphicsRegion graphics_region;
//
//                // We expect a buffer of the right size so we shouldn't need to limit repaints
//                // but we also to avoid an infinite loop.
//                int repaint_limit = 3;
//
//                do
//                {
//                    mir_buffer_stream_get_graphics_region(found->stream, &graphics_region);
//                    render_background(found->stream, graphics_region);
//                }
//                while ((new_width != graphics_region.width || new_height != graphics_region.height)
//                       && --repaint_limit != 0);
//            });
//        break;
//    }
//
//    default:
//        break;
//    }
//}
