/*
 * Copyright © 2016-2017 Canonical Ltd.
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

#include <mir/client/display_config.h>
#include <mir/client/window_spec.h>

#include <mir_toolkit/mir_buffer_stream.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <locale>
#include <codecvt>
#include <string>
#include <cstring>
#include <sstream>

#include <iostream>

namespace
{
int const title_bar_height = 12;
char const* const wallpaper_name = "wallpaper";

void null_window_callback(MirWindow*, void*) {}

struct preferred_codecvt : std::codecvt_byname<wchar_t, char, std::mbstate_t>
{
    preferred_codecvt() : std::codecvt_byname<wchar_t, char, std::mbstate_t>("") {}
    ~preferred_codecvt() = default;
};

struct Printer
{
    Printer();
    ~Printer();
    Printer(Printer const&) = delete;
    Printer& operator=(Printer const&) = delete;

    void print(MirGraphicsRegion const& region, std::string const& title, int const intensity);
    void printhelp(MirGraphicsRegion const& region);

private:
    std::wstring_convert<preferred_codecvt> converter;

    bool working = false;
    FT_Library lib;
    FT_Face face;
};

void paint_surface(MirBufferStream* buffer_stream, std::string const& title, int const intensity)
{
    // TODO sometimes buffer_stream is nullptr - find out why (and fix).
    // (Only observed when creating a lot of clients at once)
    if (!buffer_stream)
        return;

    MirGraphicsRegion region;
    mir_buffer_stream_get_graphics_region(buffer_stream, &region);

    char* row = region.vaddr;

    for (int j = 0; j != region.height; ++j)
    {
        memset(row, intensity, 4*region.width);
        row += region.stride;
    }

    static Printer printer;
    printer.print(region, title, intensity);

    mir_buffer_stream_swap_buffers_sync(buffer_stream);
}

Printer::Printer()
{
    if (FT_Init_FreeType(&lib))
        return;

    if (FT_New_Face(lib, titlebar::font_file().c_str(), 0, &face))
    {
        std::cerr << "WARNING: failed to load titlebar font: \"" <<  titlebar::font_file() << "\"\n";
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

void Printer::print(MirGraphicsRegion const& region, std::string const& title_, int const intensity)
try
{
    if (!working)
        return;

    auto title = converter.from_bytes(title_);

    int base_x = 2;
    int base_y = region.height-2;

    for (auto const& ch : title)
    {
        FT_Load_Glyph(face, FT_Get_Char_Index(face, ch), FT_LOAD_DEFAULT);
        auto const glyph = face->glyph;
        FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);

        auto const& bitmap = glyph->bitmap;
        auto const x = base_x + glyph->bitmap_left;

        if (static_cast<int>(x + bitmap.width) <= region.width)
        {
            unsigned char* src = bitmap.buffer;

            auto const y = base_y - glyph->bitmap_top;
            char* dest = region.vaddr + y*region.stride + 4*x;

            for (auto row = 0u; row != std::min(bitmap.rows, glyph->bitmap_top+2u); ++row)
            {
                for (auto col = 0u; col != bitmap.width; ++col)
                    memset(dest+ 4*col, (intensity*(0xff^src[col]))/0xff, 4);

                src += bitmap.pitch;
                dest += region.stride;
            }
        }

        base_x += glyph->advance.x >> 6;
        base_y += glyph->advance.y >> 6;
    }
}
catch (...)
{
    std::cerr << "WARNING: failed render title: \"" <<  title_ << "\"\n";
}

void Printer::printhelp(MirGraphicsRegion const& region)
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

        auto const fwidth = std::min(region.width / 60, 20);

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

    int base_y = (region.height - help_height)/2;
    auto* const region_address = reinterpret_cast<char unsigned*>(region.vaddr);

    for (auto const* rawline : helptext)
    {
        int base_x = (region.width - help_width)/2;

        auto const line = converter.from_bytes(rawline);

        for (auto const& ch : line)
        {
            FT_Load_Glyph(face, FT_Get_Char_Index(face, ch), FT_LOAD_DEFAULT);
            auto const glyph = face->glyph;
            FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);

            auto const& bitmap = glyph->bitmap;
            auto const x = base_x + glyph->bitmap_left;

            if (static_cast<int>(x + bitmap.width) <= region.width)
            {
                unsigned char* src = bitmap.buffer;

                auto const y = base_y - glyph->bitmap_top;
                auto* dest = region_address + y * region.stride + 4 * x;

                for (auto row = 0u; row != bitmap.rows; ++row)
                {
                    for (auto col = 0u; col != 4 * bitmap.width; ++col)
                    {
                        dest[col] = (0xff*src[col / 4] + (dest[col] * (0xff - src[col / 4])))/0xff;
                    }

                    src += bitmap.pitch;
                    dest += region.stride;

                    if (dest > region_address + region.height * region.stride)
                        break;
                }
            }

            base_x += glyph->advance.x >> 6;
        }
        base_y += line_height;
    }
}

void render_background(MirBufferStream* buffer_stream, MirGraphicsRegion& graphics_region)
{
    {
        uint8_t const bottom_colour[] = { 0x20, 0x54, 0xe9 };   // Ubuntu orange
        uint8_t const top_colour[] =    { 0x33, 0x33, 0x33 };   // Cool grey

        char* row = graphics_region.vaddr;

        for (int j = 0; j < graphics_region.height; j++)
        {
            uint8_t pattern[4];

            for (auto i = 0; i != 3; ++i)
                pattern[i] = (j*bottom_colour[i] + (graphics_region.height-j)*top_colour[i])/graphics_region.height;
            pattern[3] = 0xff;

            uint32_t* pixel = (uint32_t*)row;
            for (int i = 0; i < graphics_region.width; i++)
                memcpy(pixel + i, pattern, sizeof pixel[i]);

            row += graphics_region.stride;
        }
    }

    static Printer printer;
    printer.printhelp(*&graphics_region);

    mir_buffer_stream_swap_buffers_sync(buffer_stream);
}
}

using namespace mir::client;
using namespace mir::geometry;

DecorationProvider::DecorationProvider(miral::WindowManagerTools const& tools) : tools{tools}
{

}

DecorationProvider::~DecorationProvider()
{
}

void DecorationProvider::stop()
{
    enqueue_work([this]
        {
            std::lock_guard<decltype(mutex)> lock{mutex};
            window_to_titlebar.clear();
        });

    enqueue_work([this]
        {
            if (connection)
            {
                wallpaper.erase(begin(wallpaper), end(wallpaper));
            }
            connection.reset();
        });
    stop_work();
}

void DecorationProvider::operator()(Connection connection)
{
    this->connection = connection;

    DisplayConfig const display_conf{this->connection};

    display_conf.for_each_output([this](MirOutput const* output)
        {
            if (!mir_output_is_enabled(output))
                return;

            auto const mode = mir_output_get_current_mode(output);
            auto const output_id = mir_output_get_id(output);
            auto const width = mir_output_mode_get_width(mode);
            auto const height = mir_output_mode_get_height(mode);

            Surface surface{mir_connection_create_render_surface_sync(DecorationProvider::connection, width, height)};

            auto const buffer_stream =
                mir_render_surface_get_buffer_stream(surface, width, height, mir_pixel_format_xrgb_8888);

            auto window = WindowSpec::for_gloss(DecorationProvider::connection, width, height)
                .set_fullscreen_on_output(output_id)
                .set_event_handler(&handle_event_for_background, this)
                .add_surface(surface, width, height, 0, 0)
                .set_name(wallpaper_name).create_window();

            wallpaper.push_back(Wallpaper{surface, window, buffer_stream});

            MirGraphicsRegion graphics_region;
            mir_buffer_stream_get_graphics_region(buffer_stream, &graphics_region);
            render_background(buffer_stream, graphics_region);
        });

    start_work();
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

void DecorationProvider::handle_event(MirWindow* window, MirEvent const* ev, void* context_)
{
    auto* const context = (Data*)context_;

    switch (mir_event_get_type(ev))
    {
    case mir_event_type_resize:
    {
        MirResizeEvent const* resize = mir_event_get_resize_event(ev);
        int const new_width = mir_resize_event_get_width(resize);
        int const new_height = mir_resize_event_get_height(resize);
        mir_render_surface_set_size(context->surface, new_width, new_height);
        WindowSpec::for_changes(context->connection)
            .add_surface(context->surface, new_width, new_height, 0, 0)
            .apply_to(window);
        break;
    }

    default:
        break;
    }
}

void DecorationProvider::create_titlebar_for(miral::Window const& window)
{
    if (is_decoration(window)) return;

    enqueue_work([this, window]
        {
            auto const width = window.size().width.as_int();
            std::ostringstream buffer;
            buffer << std::shared_ptr<mir::scene::Surface>(window).get();

            Surface surface{mir_connection_create_render_surface_sync(this->connection, width, title_bar_height)};

            std::lock_guard<decltype(mutex)> lock{mutex};

            auto const data = &window_to_titlebar[window];
            data->connection = connection;
            data->surface = surface;
            data->stream = mir_render_surface_get_buffer_stream(surface, width, title_bar_height, mir_pixel_format_xrgb_8888);
            windows_awaiting_titlebar[buffer.str()] = window;

            WindowSpec::for_gloss(connection, width, title_bar_height)
                .add_surface(surface, width, title_bar_height, 0, 0)
                .set_name(buffer.str().c_str())
                .set_event_handler(&handle_event, data)
                .create_window(insert, data);
        });
}

void DecorationProvider::paint_titlebar_for(miral::WindowInfo const& info, int intensity)
{
    if (auto data = find_titlebar_data(info.window()))
    {
        data->intensity = intensity;

        auto const title = info.name();

        enqueue_work([stream=data->stream, title, intensity]{ paint_surface(stream, title, intensity); });
    }
}

void DecorationProvider::destroy_titlebar_for(miral::Window const& window)
{
    if (auto data = find_titlebar_data(window))
    {
        if (auto surface = data->titlebar.exchange(nullptr))
        {
            enqueue_work([surface]
                 {
                     mir_window_release(surface, &null_window_callback, nullptr);
                 });
        }

        if (data->titlebar.load())
        {
            enqueue_work([this, window]
                {
                    std::lock_guard<decltype(mutex)> lock{mutex};
                    window_to_titlebar.erase(window);
                });
        }
        else
        {
            data->on_create = [this, window](MirWindow*)
                {
                    enqueue_work([this, window]
                        {
                            std::lock_guard<decltype(mutex)> lock{mutex};
                            window_to_titlebar.erase(window);
                        });
                };
        }
    }
}

void DecorationProvider::resize_titlebar_for(miral::WindowInfo const& window_info, Size const& size)
{
    auto const window = window_info.window();

    if (window.size().width == size.width)
        return;

    if (auto titlebar_window = find_titlebar_window(window))
    {
        titlebar_window.resize({size.width, title_bar_height});

        repaint_titlebar_for(window_info);
    }
}

void DecorationProvider::place_new_decoration(miral::WindowSpecification& window_spec)
{
    auto const name = window_spec.name().value();
    if (name == wallpaper_name) return;

    std::lock_guard<decltype(mutex)> lock{mutex};

    auto const scene_surface = windows_awaiting_titlebar[name].lock();
    windows_awaiting_titlebar.erase(name);

    auto& parent_info = tools.info_for(scene_surface);
    auto const parent_window = parent_info.window();

    window_spec.parent() = scene_surface;
    window_spec.size() = Size{parent_window.size().width, Height{title_bar_height}};
    window_spec.top_left() = parent_window.top_left() - Displacement{0, title_bar_height};
}

void DecorationProvider::advise_new_titlebar(miral::WindowInfo const& window_info)
{
    if (window_info.name() == wallpaper_name) return;

    {
        std::lock_guard<decltype(mutex)> lock{mutex};

        window_to_titlebar[window_info.parent()].window = window_info.window();
    }

    tools.raise_tree(window_info.parent());
}

void DecorationProvider::advise_state_change(miral::WindowInfo const& window_info, MirWindowState state)
{
    if (auto titlebar = find_titlebar_window(window_info.window()))
    {
        miral::WindowSpecification modifications;
        switch (state)
        {
        case mir_window_state_hidden:
        case mir_window_state_minimized:
        case mir_window_state_fullscreen:
            modifications.state() = mir_window_state_hidden;
            break;

        default:
            modifications.state() = mir_window_state_restored;
            break;
        }

        tools.modify_window(titlebar, modifications);
        repaint_titlebar_for(window_info);
    }
}

void DecorationProvider::repaint_titlebar_for(miral::WindowInfo const& window_info)
{
    if (auto data = find_titlebar_data(window_info.window()))
    {
        auto const title = window_info.name();

        enqueue_work([stream=data->stream, title, intensity=data->intensity.load()]
            { paint_surface(stream, title, intensity); });
    }
}

DecorationProvider::Data::~Data()
{
    if (auto surface = titlebar.exchange(nullptr))
        mir_window_release(surface, &null_window_callback, nullptr);
}

void DecorationProvider::insert(MirWindow* surface, Data* data)
{
    data->on_create(surface);
    data->titlebar = surface;
}

DecorationProvider::Data* DecorationProvider::find_titlebar_data(miral::Window const& window)
{
    std::lock_guard<decltype(mutex)> lock{mutex};

    auto const find = window_to_titlebar.find(window);

    return (find != window_to_titlebar.end()) ? &find->second : nullptr;
}

miral::Window DecorationProvider::find_titlebar_window(miral::Window const& window) const
{
    std::lock_guard<decltype(mutex)> lock{mutex};

    auto const find = window_to_titlebar.find(window);

    return (find != window_to_titlebar.end()) ? find->second.window : miral::Window{};
}

bool DecorationProvider::is_decoration(miral::Window const& window) const
{
    return window.application() == session();
}

bool DecorationProvider::is_titlebar(miral::WindowInfo const& window_info) const
{
    return window_info.window().application() == session() && window_info.name() != wallpaper_name;
}

void DecorationProvider::handle_event_for_background(MirWindow* window, MirEvent const* event, void* context)
{
    static_cast<DecorationProvider*>(context)->handle_event_for_background(window, event);
}

void DecorationProvider::handle_event_for_background(MirWindow* window, MirEvent const* ev)
{
    switch (mir_event_get_type(ev))
    {
    case mir_event_type_resize:
    {
        MirResizeEvent const* resize = mir_event_get_resize_event(ev);
        int const new_width = mir_resize_event_get_width(resize);
        int const new_height = mir_resize_event_get_height(resize);

        enqueue_work([window, new_width, new_height, this]()
            {
                auto found = find_if(begin(wallpaper), end(wallpaper), [&](Wallpaper const& w) { return w.window == window;});
                if (found == end(wallpaper)) return;

                mir_buffer_stream_set_size(found->stream, new_width, new_height);
                mir_render_surface_set_size(found->surface, new_width, new_height);

                WindowSpec::for_changes(connection)
                    .add_surface(found->surface, new_width, new_height, 0, 0)
                    .apply_to(window);

                MirGraphicsRegion graphics_region;

                // We expect a buffer of the right size so we shouldn't need to limit repaints
                // but we also to avoid an infinite loop.
                int repaint_limit = 3;

                do
                {
                    mir_buffer_stream_get_graphics_region(found->stream, &graphics_region);
                    render_background(found->stream, graphics_region);
                }
                while ((new_width != graphics_region.width || new_height != graphics_region.height)
                       && --repaint_limit != 0);
            });
        break;
    }

    default:
        break;
    }
}

Worker::~Worker()
{
}

void Worker::do_work()
{
    while (!work_done)
    {
        WorkQueue::value_type work;
        {
            std::unique_lock<decltype(work_mutex)> lock{work_mutex};
            work_cv.wait(lock, [this] { return !work_queue.empty(); });
            work = work_queue.front();
            work_queue.pop();
        }

        work();
    }
}

void Worker::enqueue_work(std::function<void()> const& functor)
{
    std::lock_guard<decltype(work_mutex)> lock{work_mutex};
    work_queue.push(functor);
    work_cv.notify_one();
}

void Worker::start_work()
{
    do_work();
}

void Worker::stop_work()
{
    enqueue_work([this] { work_done = true; });
}
