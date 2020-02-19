/*
 * Copyright © 2019 Canonical Ltd.
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
 *
 * Authored By: William Wold <william.wold@canonical.com>
 */


#include "renderer.h"
#include "window.h"
#include "input.h"

#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/geometry/displacement.h"
#include "mir/log.h"

#include <boost/throw_exception.hpp>
#include <boost/filesystem.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <locale>
#include <codecvt>

namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;
namespace geom = mir::geometry;
namespace msh = mir::shell;
namespace msd = mir::shell::decoration;

namespace
{
static constexpr auto color(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 0xFF) -> uint32_t
{
    return ((uint32_t)b <<  0) |
           ((uint32_t)g <<  8) |
           ((uint32_t)r << 16) |
           ((uint32_t)a << 24);
}

uint32_t const default_focused_background   = color(0x32, 0x32, 0x32);
uint32_t const default_unfocused_background = color(0x80, 0x80, 0x80);
uint32_t const default_focused_text         = color(0xFF, 0xFF, 0xFF);
uint32_t const default_unfocused_text       = color(0xA0, 0xA0, 0xA0);
uint32_t const default_normal_button        = color(0x60, 0x60, 0x60);
uint32_t const default_active_button        = color(0xA0, 0xA0, 0xA0);
uint32_t const default_close_normal_button  = color(0xA0, 0x20, 0x20);
uint32_t const default_close_active_button  = color(0xC0, 0x60, 0x60);
uint32_t const default_button_icon          = color(0xFF, 0xFF, 0xFF);

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
};

char const* const font_path_search_paths[]{
    "/usr/share/fonts/truetype",    // Ubuntu/Debian
    "/usr/share/fonts/TTF",         // Arch
    "/usr/share/fonts",             // Fedora/Arch
};

inline auto area(geom::Size size) -> size_t
{
    return (size.width > geom::Width{} && size.height > geom::Height{})
        ? size.width.as_int() * size.height.as_int()
        : 0;
}

inline void render_row(
    uint32_t* const data,
    geom::Size buf_size,
    geom::Point left,
    geom::Width length,
    uint32_t color)
{
    if (left.y < geom::Y{} || left.y >= as_y(buf_size.height))
        return;
    geom::X const right = std::min(left.x + as_delta(length), as_x(buf_size.width));
    left.x = std::max(left.x, geom::X{});
    uint32_t* const start = data + (left.y.as_int() * buf_size.width.as_int()) + left.x.as_int();
    uint32_t* const end = start + right.as_int() - left.x.as_int();
    for (uint32_t* i = start; i < end; i++)
        *i = color;
}

inline void render_close_icon(
    uint32_t* const data,
    geom::Size buf_size,
    geom::Rectangle box,
    geom::Width line_width,
    uint32_t color)
{
    for (geom::Y y = box.top(); y < box.bottom(); y += geom::DeltaY{1})
    {
        float const height = (y - box.top()).as_int() / (float)box.size.height.as_int();
        geom::X const left_line_left = box.left() + as_delta(box.size.width * height);
        geom::X const right_line_left = box.right() - as_delta(box.size.width * height) - as_delta(line_width);
        render_row(data, buf_size, {left_line_left, y}, line_width, color);
        render_row(data, buf_size, {right_line_left, y}, line_width, color);
    }
}

inline void render_maximize_icon(
    uint32_t* const data,
    geom::Size buf_size,
    geom::Rectangle box,
    geom::Width line_width,
    uint32_t color)
{
    geom::Y const mini_titlebar_bottom = box.top() + as_delta(box.size.height * 0.25);
    geom::Y const mini_bottom_border_top = box.bottom() - geom::DeltaY{line_width.as_int()};
    for (geom::Y y = box.top(); y < mini_titlebar_bottom; y += geom::DeltaY{1})
    {
        render_row(data, buf_size, {box.left(), y}, box.size.width, color);
    }
    for (geom::Y y = mini_titlebar_bottom; y < mini_bottom_border_top; y += geom::DeltaY{1})
    {
        render_row(data, buf_size, {box.left(), y}, line_width, color);
        render_row(data, buf_size, {box.right() - as_delta(line_width), y}, line_width, color);
    }
    for (geom::Y y = mini_bottom_border_top; y < box.bottom(); y += geom::DeltaY{1})
    {
        render_row(data, buf_size, {box.left(), y}, box.size.width, color);
    }
}

inline void render_minimize_icon(
    uint32_t* const data,
    geom::Size buf_size,
    geom::Rectangle box,
    geom::Width /*line_width*/,
    uint32_t color)
{
    geom::Y const mini_taskbar_top = box.bottom() - as_delta(box.size.height * 0.25);
    geom::Width const mini_taskbar_size = box.size.width * 0.6;
    for (geom::Y y = mini_taskbar_top; y < box.bottom(); y += geom::DeltaY{1})
    {
        render_row(data, buf_size, {box.left(), y}, mini_taskbar_size, color);
    }
}
}

class msd::Renderer::Text::Impl
    : public Text
{
public:
    Impl();
    ~Impl();

    void render(
        Pixel* buf,
        geom::Size buf_size,
        std::string const& text,
        geom::Point top_left,
        geom::Height height_pixels,
        Pixel color) override;

private:
    std::mutex mutex;
    FT_Library library;
    FT_Face face;

    void set_char_size(geom::Height height);
    void rasterize_glyph(char32_t glyph);
    void render_glyph(
        Pixel* buf,
        geom::Size buf_size,
        FT_Bitmap const* glyph,
        geom::Point top_left,
        Pixel color);

    static auto font_path() -> std::string;
    static auto utf8_to_utf32(std::string const& text) -> std::u32string;
};

class msd::Renderer::Text::Null
    : public Text
{
public:
    void render(
        Pixel*,
        geom::Size,
        std::string const&,
        geom::Point,
        geom::Height,
        Pixel) override
    {
    }

private:
};

std::mutex msd::Renderer::Text::static_mutex;
std::weak_ptr<msd::Renderer::Text> msd::Renderer::Text::singleton;

auto msd::Renderer::Text::instance() -> std::shared_ptr<Text>
{
    std::lock_guard<std::mutex> lock{static_mutex};
    auto shared = singleton.lock();
    if (!shared)
    {
        try
        {
            shared = std::make_shared<Impl>();
        }
        catch (std::runtime_error const& error)
        {
            log_warning(error.what());
            shared = std::make_shared<Null>();
        }
        singleton = shared;
    }
    return shared;
}

msd::Renderer::Text::Impl::Impl()
{
    if (auto const error = FT_Init_FreeType(&library))
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Initializing freetype library failed with error " + std::to_string(error)));

    auto const path = font_path();
    if (auto const error = FT_New_Face(library, path.c_str(), 0, &face))
    {
        if (error == FT_Err_Unknown_File_Format)
            BOOST_THROW_EXCEPTION(std::runtime_error(
                "Font " + path + " has unsupported format"));
        else
            BOOST_THROW_EXCEPTION(std::runtime_error(
                "Loading font from " + path + " failed with error " + std::to_string(error)));
    }
}

msd::Renderer::Text::Impl::~Impl()
{
    if (auto const error = FT_Done_Face(face))
        log_warning("Failed to uninitialize font face with error %d", error);
    face = nullptr;

    if (auto const error = FT_Done_FreeType(library))
        log_warning("Failed to uninitialize FreeType with error %d", error);
    library = nullptr;
}

void msd::Renderer::Text::Impl::render(
    Pixel* buf,
    geom::Size buf_size,
    std::string const& text,
    geom::Point top_left,
    geom::Height height_pixels,
    Pixel color)
{
    if (!area(buf_size) || height_pixels <= geom::Height{})
        return;

    std::lock_guard<std::mutex> lock{mutex};

    if (!library || !face)
    {
        log_warning("FreeType not initialized");
        return;
    }

    try
    {
        set_char_size(height_pixels);
    }
    catch (std::runtime_error const& error)
    {
        log_warning(error.what());
        return;
    }

    auto const utf32 = utf8_to_utf32(text);

    for (char32_t const glyph : utf32)
    {
        try
        {
            rasterize_glyph(glyph);

            geom::Point glyph_top_left =
                top_left +
                geom::Displacement{
                    face->glyph->bitmap_left,
                    height_pixels.as_int() - face->glyph->bitmap_top};
            render_glyph(buf, buf_size, &face->glyph->bitmap, glyph_top_left, color);

            top_left += geom::Displacement{
                face->glyph->advance.x / 64,
                face->glyph->advance.y / 64};
        }
        catch (std::runtime_error const& error)
        {
            log_warning(error.what());
        }
    }
}

void msd::Renderer::Text::Impl::set_char_size(geom::Height height)
{
    if (auto const error = FT_Set_Pixel_Sizes(face, 0, height.as_int()))
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Setting char size failed with error " + std::to_string(error)));
}

void msd::Renderer::Text::Impl::rasterize_glyph(char32_t glyph)
{
    auto const glyph_index = FT_Get_Char_Index(face, glyph);

    if (auto const error = FT_Load_Glyph(face, glyph_index, 0))
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Failed to load glyph " + std::to_string(glyph_index)));

    if (auto const error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL))
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Failed to render glyph " + std::to_string(glyph_index)));
}

void msd::Renderer::Text::Impl::render_glyph(
    Pixel* buf,
    geom::Size buf_size,
    FT_Bitmap const* glyph,
    geom::Point top_left,
    Pixel color)
{
    geom::X const buffer_left = std::max(top_left.x, geom::X{});
    geom::X const buffer_right = std::min(top_left.x + geom::DeltaX{glyph->width}, as_x(buf_size.width));

    geom::Y const buffer_top = std::max(top_left.y, geom::Y{});
    geom::Y const buffer_bottom = std::min(top_left.y + geom::DeltaY{glyph->rows}, as_y(buf_size.height));

    geom::Displacement const glyph_offset = as_displacement(top_left);

    unsigned char* const color_pixels = (unsigned char *)&color;
    unsigned char const color_alpha = color_pixels[3];

    for (geom::Y buffer_y = buffer_top; buffer_y < buffer_bottom; buffer_y += geom::DeltaY{1})
    {
        geom::Y const glyph_y = buffer_y - glyph_offset.dy;
        unsigned char* const glyph_row = glyph->buffer + glyph_y.as_int() * glyph->pitch;
        Pixel* const buffer_row = buf + buffer_y.as_int() * buf_size.width.as_int();

        for (geom::X buffer_x = buffer_left; buffer_x < buffer_right; buffer_x += geom::DeltaX{1})
        {
            geom::X const glyph_x = buffer_x - glyph_offset.dx;
            unsigned char const glyph_alpha = ((int)glyph_row[glyph_x.as_int()] * color_alpha) / 255;
            unsigned char* const buffer_pixels = (unsigned char *)(buffer_row + buffer_x.as_int());
            for (int i = 0; i < 3; i++)
            {
                // Blend color with the previous buffer color based on the glyph's alpha
                buffer_pixels[i] =
                    ((int)buffer_pixels[i] * (255 - glyph_alpha)) / 255 +
                    ((int)color_pixels[i] * glyph_alpha) / 255;
            }
        }
    }
}

auto msd::Renderer::Text::Impl::font_path() -> std::string
{
    // Similar to default_font() in examples/example-server-lib/wallpaper_config.cpp

    std::vector<std::string> usable_search_paths;
    for (auto const& path : font_path_search_paths)
    {
        if (boost::filesystem::exists(path))
            usable_search_paths.push_back(path);
    }

    for (auto const& font : font_paths)
    {
        for (auto const& prefix : font.prefixes)
        {
            for (auto const& path : usable_search_paths)
            {
                auto const full_font_path = path + '/' + prefix + '/' + font.filename;
                if (boost::filesystem::exists(full_font_path))
                    return full_font_path;
            }
        }
    }

    BOOST_THROW_EXCEPTION(std::runtime_error("Failed to find a font"));
}

auto msd::Renderer::Text::Impl::utf8_to_utf32(std::string const& text) -> std::u32string
{
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    std::u32string utf32_text;
    try {
        utf32_text = converter.from_bytes(text);
    } catch(const std::range_error& e) {
        log_warning("Window title %s is not valid UTF-8", text.c_str());
        // fall back to ASCII
        for (char const c : text)
        {
            if (isprint(c))
                utf32_text += c;
            else
                utf32_text += 0xFFFD; // REPLACEMENT CHARACTER (�)
        }
    }
    return utf32_text;
}

msd::Renderer::Renderer(
    std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<StaticGeometry const> const& static_geometry)
    : buffer_allocator{buffer_allocator},
      focused_theme{
          default_focused_background,
          default_focused_text},
      unfocused_theme{
          default_unfocused_background,
          default_unfocused_text},
      current_theme{nullptr},
      button_icons{
          {ButtonFunction::Close, {
              default_close_normal_button,
              default_close_active_button,
              default_button_icon,
              render_close_icon}},
          {ButtonFunction::Maximize, {
              default_normal_button,
              default_active_button,
              default_button_icon,
              render_maximize_icon}},
          {ButtonFunction::Minimize, {
              default_normal_button,
              default_active_button,
              default_button_icon,
              render_minimize_icon}},
      },
      static_geometry{static_geometry},
      text{Text::instance()}
{
}

void msd::Renderer::update_state(WindowState const& window_state, InputState const& input_state)
{
    left_border_size = window_state.left_border_rect().size;
    right_border_size = window_state.right_border_rect().size;
    bottom_border_size = window_state.bottom_border_rect().size;

    size_t length{0};
    length = std::max(length, area(left_border_size));
    length = std::max(length, area(right_border_size));
    length = std::max(length, area(bottom_border_size));

    if (length != solid_color_pixels_length)
    {
        solid_color_pixels_length = length;
        solid_color_pixels.reset(); // force a reallocation next time it's needed
    }

    if (window_state.titlebar_rect().size != titlebar_size)
    {
        titlebar_size = window_state.titlebar_rect().size;
        titlebar_pixels.reset(); // force a reallocation next time it's needed
    }

    Theme const* const new_theme = (window_state.focused_state() == mir_window_focus_state_focused) ?
        &focused_theme :
        &unfocused_theme;

    if (new_theme != current_theme)
    {
        current_theme = new_theme;
        needs_titlebar_redraw = true;
        needs_solid_color_redraw = true;
    }

    if (window_state.window_name() != name)
    {
        name = window_state.window_name();
        needs_titlebar_redraw = true;
    }

    if (input_state.buttons() != buttons)
    {
        // If the number of buttons or their location changed, redraw the whole titlebar
        // Otherwise if the buttons are in the same place, just redraw them
        if (input_state.buttons().size() != buttons.size())
        {
            needs_titlebar_redraw = true;
        }
        else
        {
            for (unsigned i = 0; i < buttons.size(); i++)
            {
                if (input_state.buttons()[i].rect != buttons[i].rect)
                    needs_titlebar_redraw = true;
            }
        }
        buttons = input_state.buttons();
        needs_titlebar_buttons_redraw = true;
    }
}

auto msd::Renderer::render_titlebar() -> std::experimental::optional<std::shared_ptr<mg::Buffer>>
{
    if (!area(titlebar_size))
        return std::experimental::nullopt;

    if (!titlebar_pixels)
    {
        titlebar_pixels = alloc_pixels(titlebar_size);
        needs_titlebar_redraw = true;
    }

    if (needs_titlebar_redraw)
    {
        for (geom::Y y{0}; y < as_y(titlebar_size.height); y += geom::DeltaY{1})
        {
            render_row(
                titlebar_pixels.get(), titlebar_size,
                {0, y}, titlebar_size.width,
                current_theme->background_color);
        }

        text->render(
            titlebar_pixels.get(),
            titlebar_size,
            name,
            static_geometry->title_font_top_left,
            static_geometry->title_font_height,
            current_theme->text_color);
    }

    if (needs_titlebar_redraw || needs_titlebar_buttons_redraw)
    {
        for (auto const& button : buttons)
        {
            auto const icon = button_icons.find(button.function);
            if (icon != button_icons.end())
            {
                Pixel button_color = icon->second.normal_color;
                if (button.state == ButtonState::Hovered)
                    button_color = icon->second.active_color;
                for (geom::Y y{button.rect.top()}; y < button.rect.bottom(); y += geom::DeltaY{1})
                {
                    render_row(
                        titlebar_pixels.get(),
                        titlebar_size,
                        {button.rect.left(), y},
                        button.rect.size.width,
                        button_color);
                }
                geom::Rectangle const icon_rect = {
                button.rect.top_left + static_geometry->icon_padding, {
                    button.rect.size.width - static_geometry->icon_padding.dx * 2,
                    button.rect.size.height - static_geometry->icon_padding.dy * 2}};
                icon->second.render_icon(
                    titlebar_pixels.get(),
                    titlebar_size,
                    icon_rect,
                    static_geometry->icon_line_width,
                    icon->second.icon_color);
            }
            else
            {
                log_warning("Could not render decoration button with unknown function %d\n", button.function);
            }
        }
    }

    needs_titlebar_redraw = false;
    needs_titlebar_buttons_redraw = false;

    return make_buffer(titlebar_pixels.get(), titlebar_size);
}

auto msd::Renderer::render_left_border() -> std::experimental::optional<std::shared_ptr<mg::Buffer>>
{
    if (!area(left_border_size))
        return std::experimental::nullopt;
    update_solid_color_pixels();
    return make_buffer(solid_color_pixels.get(), left_border_size);
}

auto msd::Renderer::render_right_border() -> std::experimental::optional<std::shared_ptr<mg::Buffer>>
{
    if (!area(right_border_size))
        return std::experimental::nullopt;
    update_solid_color_pixels();
    return make_buffer(solid_color_pixels.get(), right_border_size);
}

auto msd::Renderer::render_bottom_border() -> std::experimental::optional<std::shared_ptr<mg::Buffer>>
{
    if (!area(bottom_border_size))
        return std::experimental::nullopt;
    update_solid_color_pixels();
    return make_buffer(solid_color_pixels.get(), bottom_border_size);
}

void msd::Renderer::update_solid_color_pixels()
{
    if (!solid_color_pixels)
    {
        solid_color_pixels = alloc_pixels(geom::Size{solid_color_pixels_length, 1});
        needs_solid_color_redraw = true;
    }

    if (needs_solid_color_redraw)
    {
        render_row(
            solid_color_pixels.get(), geom::Size{solid_color_pixels_length, 1},
            geom::Point{}, geom::Width{solid_color_pixels_length},
            current_theme->background_color);
    }

    needs_solid_color_redraw = false;
}

auto msd::Renderer::make_buffer(
    uint32_t const* pixels,
    geometry::Size size) -> std::experimental::optional<std::shared_ptr<mg::Buffer>>
{
    if (!area(size))
    {
        log_warning("Failed to draw SSD: tried to create zero size buffer");
        return std::experimental::nullopt;
    }
    std::shared_ptr<graphics::Buffer> const buffer = buffer_allocator->alloc_software_buffer(size, buffer_format);
    std::shared_ptr<mrs::WriteMappableBuffer> writable;
    try
    {
        writable = mrs::as_write_mappable_buffer(buffer);
    }
    catch (std::runtime_error const&)
    {
        log_warning("Failed to draw SSD: software buffer not a pixel source");
        return std::experimental::nullopt;
    }
    auto const mapping = writable->map_writeable();
    ::memcpy(
        mapping->data(),
        reinterpret_cast<unsigned char const*>(pixels),
        mapping->len());
    return buffer;
}

auto msd::Renderer::alloc_pixels(geometry::Size size) -> std::unique_ptr<uint32_t[]>
{
    size_t const buf_size = area(size) * bytes_per_pixel;
    if (buf_size)
        return std::unique_ptr<uint32_t[]>{new uint32_t[buf_size]};
    else
        return nullptr;
}
