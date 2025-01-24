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


#include "decoration_strategy.h"
#include "window.h"

#include "mir/fatal.h"
#include "mir/geometry/displacement.h"
#include "mir/log.h"

#include <boost/throw_exception.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <endian.h>

#include <locale>
#include <codecvt>
#include <filesystem>
#include <map>

namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace msh = mir::shell;
namespace msd = mir::shell::decoration;

using msd::WindowState;
using msd::Pixel;
using msd::InputState;

namespace
{
/// Decoration geometry properties that don't change
struct StaticGeometry
{
    geom::Height const titlebar_height;           ///< Visible height of the top titlebar with the window's name and buttons
    geom::Width const side_border_width;          ///< Visible width of the side borders
    geom::Height const bottom_border_height;      ///< Visible height of the bottom border
    geom::Size const resize_corner_input_size;    ///< The size of the input area of a resizable corner
    ///< (does not effect surface input area, only where in the surface is
    ///< considered a resize corner)
    geom::Width const button_width;               ///< The width of window control buttons
    geom::Width const padding_between_buttons;    ///< The gep between titlebar buttons
    geom::Height const title_font_height;         ///< Height of the text used to render the window title
    geom::Point const title_font_top_left;        ///< Where to render the window title
    geom::Displacement const icon_padding;        ///< Padding inside buttons around icons
    geom::Width const icon_line_width;            ///< Width for lines in button icons

    MirPixelFormat const buffer_format = mir_pixel_format_argb_8888;
};

static constexpr auto color(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 0xFF) -> uint32_t
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return ((uint32_t)b <<  0) |
           ((uint32_t)g <<  8) |
           ((uint32_t)r << 16) |
           ((uint32_t)a << 24);
#elif __BYTE_ORDER == __BIG_ENDIAN
    return ((uint32_t)b << 24) |
           ((uint32_t)g << 16) |
           ((uint32_t)r <<  8) |
           ((uint32_t)a <<  0);
#else
#error unsupported byte order
#endif
}

uint32_t constexpr default_focused_background   = color(0x32, 0x32, 0x32);
uint32_t constexpr default_unfocused_background = color(0x80, 0x80, 0x80);
uint32_t constexpr default_focused_text         = color(0xFF, 0xFF, 0xFF);
uint32_t constexpr default_unfocused_text       = color(0xA0, 0xA0, 0xA0);
uint32_t constexpr default_normal_button        = color(0x60, 0x60, 0x60);
uint32_t constexpr default_active_button        = color(0xA0, 0xA0, 0xA0);
uint32_t constexpr default_close_normal_button  = color(0xA0, 0x20, 0x20);
uint32_t constexpr default_close_active_button  = color(0xC0, 0x60, 0x60);
uint32_t constexpr default_button_icon          = color(0xFF, 0xFF, 0xFF);

/// Font search logic should be kept in sync with examples/example-server-lib/wallpaper_config.cpp
auto default_font() -> std::string
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
            "liberation",           // Arch
        }},
    };

    char const* const font_path_search_paths[]{
        "/usr/share/fonts/truetype",    // Ubuntu/Debian
        "/usr/share/fonts/TTF",         // Arch
        "/usr/share/fonts",             // Fedora/Arch
    };

    std::vector<std::filesystem::path> usable_search_paths;
    for (auto const& path : font_path_search_paths)
    {
        if (std::filesystem::exists(path))
            usable_search_paths.push_back(path);
    }

    for (auto const& font : font_paths)
    {
        for (auto const& prefix : font.prefixes)
        {
            for (auto const& path : usable_search_paths)
            {
                auto const full_font_path = path / prefix / font.filename;
                if (std::filesystem::exists(full_font_path))
                    return full_font_path;
            }
        }
    }

    return "";
}

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

struct RendererStrategy : public msd::RendererStrategy
{
    RendererStrategy(std::shared_ptr<StaticGeometry> const& static_geometry);

    void update_state(WindowState const& window_state, InputState const& input_state) override;
    auto render_titlebar(msd::BufferMaker const* buffer_maker) -> std::optional<std::shared_ptr<mir::graphics::Buffer>> override;
    auto render_left_border(msd::BufferMaker const* buffer_maker) -> std::optional<std::shared_ptr<mir::graphics::Buffer>> override;
    auto render_right_border(msd::BufferMaker const* buffer_maker) -> std::optional<std::shared_ptr<mir::graphics::Buffer>> override;
    auto render_bottom_border(msd::BufferMaker const* buffer_maker) -> std::optional<std::shared_ptr<mir::graphics::Buffer>> override;

private:
    std::shared_ptr<StaticGeometry> const static_geometry;

    /// A visual theme for a decoration
    /// Focused and unfocused windows use a different theme
    struct Theme
    {
        Pixel const background_color;   ///< Color for background of the titlebar and borders
        Pixel const text_color;         ///< Color the window title is drawn in
    };
    Theme const focused_theme;
    Theme const unfocused_theme;
    Theme const* current_theme;

    class Text
    {
    public:
        static auto instance() -> std::shared_ptr<Text>;

        virtual ~Text() = default;

        virtual void render(
            Pixel* buf,
            geom::Size buf_size,
            std::string const& text,
            geom::Point top_left,
            geom::Height height_pixels,
            Pixel color) = 0;

    private:
        class Impl;
        class Null;

        static std::mutex static_mutex;
        static std::weak_ptr<Text> singleton;
    };
    std::shared_ptr<Text> const text;

    // Info needed to render a button icon
    struct Icon
    {
        Pixel const normal_color;   ///< Normal of the background of the button area
        Pixel const active_color;   ///< Color of the background when button is active
        Pixel const icon_color;     ///< Color of the icon
        std::function<void(
            Pixel* const data,
            geom::Size buf_size,
            geom::Rectangle box,
            geom::Width line_width,
            Pixel color)> const render_icon; ///< Draws button's icon to the given buffer
    };
    std::map<msd::Button::Function, Icon const> button_icons;

    float scale{1.0f};
    std::string name;
    geom::Size left_border_size;
    geom::Size right_border_size;
    geom::Size bottom_border_size;

    bool needs_solid_color_redraw{true};

    size_t solid_color_pixels_length{0};
    size_t scaled_solid_color_pixels_length{0};
    std::unique_ptr<Pixel[]> solid_color_pixels; // can be nullptr

    geom::Size titlebar_size{};
    std::unique_ptr<Pixel[]> titlebar_pixels; // can be nullptr

    bool needs_titlebar_redraw{true};
    bool needs_titlebar_buttons_redraw{true};

    std::vector<msd::Button> buttons;

    void update_solid_color_pixels();

    void set_focus_state(MirWindowFocusState focus_state);

    void redraw_titlebar_background(geom::Size scaled_titlebar_size);
    void redraw_titlebar_text(geom::Size scaled_titlebar_size);
    void redraw_titlebar_buttons(geom::Size scaled_titlebar_size);
};

class DecorationStrategy : public msd::DecorationStrategy, public std::enable_shared_from_this<DecorationStrategy>
{
public:
    ~DecorationStrategy() override = default;

    auto static_geometry() const -> std::shared_ptr<StaticGeometry>;
    auto render_strategy() const -> std::unique_ptr<mir::shell::decoration::RendererStrategy> override;
    auto button_placement(unsigned n, const WindowState& ws) const -> geom::Rectangle override;
    auto compute_size_with_decorations(geom::Size content_size, MirWindowType type, MirWindowState state) const
        -> geom::Size override;
    auto buffer_format() const -> MirPixelFormat override;
    auto new_window_state(const std::shared_ptr<mir::scene::Surface>& window_surface,
                          float scale) const -> std::unique_ptr<WindowState> override;
    auto resize_corner_input_size() const -> geom::Size override;
    auto titlebar_height() const -> geom::Height override;
    auto side_border_width() const -> geom::Width override;
    auto bottom_border_height() const -> geom::Height override;
};

auto DecorationStrategy::titlebar_height() const -> geom::Height
{
    return static_geometry()->titlebar_height;
}

auto DecorationStrategy::side_border_width() const -> geom::Width
{
    return static_geometry()->side_border_width;
}

auto DecorationStrategy::bottom_border_height() const -> geom::Height
{
    return static_geometry()->bottom_border_height;
}

auto DecorationStrategy::resize_corner_input_size() const -> geom::Size
{
    return static_geometry()->resize_corner_input_size;
}

auto DecorationStrategy::buffer_format() const -> MirPixelFormat
{
    return static_geometry()->buffer_format;
}

auto DecorationStrategy::new_window_state(const std::shared_ptr<mir::scene::Surface>& window_surface,
                                          float scale) const -> std::unique_ptr<WindowState>

{
    return std::make_unique<WindowState>(shared_from_this(), window_surface, scale);
}

auto DecorationStrategy::compute_size_with_decorations(
    geom::Size content_size, MirWindowType type, MirWindowState state) const -> geom::Size
{
    switch (msd::border_type_for(type, state))
    {
    case msd::BorderType::Full:
        content_size.width += static_geometry()->side_border_width * 2;
        content_size.height += static_geometry()->titlebar_height + static_geometry()->bottom_border_height;
        break;
    case msd::BorderType::Titlebar:
        content_size.height += static_geometry()->titlebar_height;
        break;
    case msd::BorderType::None:
        break;
    }

    return content_size;
}

auto alloc_pixels(geom::Size size) -> std::unique_ptr<Pixel[]>
{
    size_t const buf_size = area(size) * sizeof(Pixel);
    if (buf_size)
        return std::unique_ptr<Pixel[]>{new Pixel[buf_size]};
    else
        return nullptr;
}
}

auto msd::border_type_for(MirWindowType type, MirWindowState state) -> msd::BorderType
{
    using BorderType = msd::BorderType;

    switch (type)
    {
    case mir_window_type_normal:
    case mir_window_type_utility:
    case mir_window_type_dialog:
    case mir_window_type_freestyle:
    case mir_window_type_satellite:
        break;

    case mir_window_type_gloss:
    case mir_window_type_menu:
    case mir_window_type_inputmethod:
    case mir_window_type_tip:
    case mir_window_type_decoration:
    case mir_window_types:
        return BorderType::None;
    }

    switch (state)
    {
    case mir_window_state_unknown:
    case mir_window_state_restored:
        return BorderType::Full;

    case mir_window_state_maximized:
    case mir_window_state_vertmaximized:
    case mir_window_state_horizmaximized:
        return BorderType::Titlebar;

    case mir_window_state_minimized:
    case mir_window_state_fullscreen:
    case mir_window_state_hidden:
    case mir_window_state_attached:
    case mir_window_states:
        return BorderType::None;
    }

    mir::fatal_error("%s:%d: should be unreachable", __FILE__, __LINE__);
    return {};
}

auto msd::DecorationStrategy::default_decoration_strategy() -> std::shared_ptr<DecorationStrategy>
{
    return std::make_shared<::DecorationStrategy>();
}

class RendererStrategy::Text::Impl
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

class RendererStrategy::Text::Null
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

std::mutex RendererStrategy::Text::static_mutex;
std::weak_ptr<RendererStrategy::Text> RendererStrategy::Text::singleton;

auto RendererStrategy::Text::instance() -> std::shared_ptr<Text>
{
    std::lock_guard lock{static_mutex};
    auto shared = singleton.lock();
    if (!shared)
    {
        try
        {
            shared = std::make_shared<Impl>();
        }
        catch (std::runtime_error const& error)
        {
            mir::log_warning("%s", error.what());
            shared = std::make_shared<Null>();
        }
        singleton = shared;
    }
    return shared;
}

RendererStrategy::Text::Impl::Impl()
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

RendererStrategy::Text::Impl::~Impl()
{
    if (auto const error = FT_Done_Face(face))
        mir::log_warning("Failed to uninitialize font face with error %d", error);
    face = nullptr;

    if (auto const error = FT_Done_FreeType(library))
        mir::log_warning("Failed to uninitialize FreeType with error %d", error);
    library = nullptr;
}

void RendererStrategy::Text::Impl::render(
    Pixel* buf,
    geom::Size buf_size,
    std::string const& text,
    geom::Point top_left,
    geom::Height height_pixels,
    Pixel color)
{
    if (!area(buf_size) || height_pixels <= geom::Height{})
        return;

    std::lock_guard lock{mutex};

    if (!library || !face)
    {
        mir::log_warning("FreeType not initialized");
        return;
    }

    try
    {
        set_char_size(height_pixels);
    }
    catch (std::runtime_error const& error)
    {
        mir::log_warning("%s", error.what());
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
            mir::log_warning("%s", error.what());
        }
    }
}

void RendererStrategy::Text::Impl::set_char_size(geom::Height height)
{
    if (auto const error = FT_Set_Pixel_Sizes(face, 0, height.as_int()))
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Setting char size failed with error " + std::to_string(error)));
}

namespace
{
auto freetype_error_to_string(FT_Error error) -> char const*
{
    auto reason = FT_Error_String(error);
    /* The reason is only available if freetype has been built with a specific feature enabled */
    if (!reason)
    {
        return "<unknown error>";
    }
    return reason;
}
}

void RendererStrategy::Text::Impl::rasterize_glyph(char32_t glyph)
{
    auto const glyph_index = FT_Get_Char_Index(face, glyph);

    if (auto const error = FT_Load_Glyph(face, glyph_index, 0))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Failed to load glyph " + std::to_string(glyph_index) +
            " (" + freetype_error_to_string(error) + ")"));
    }

    if (auto const error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Failed to render glyph " + std::to_string(glyph_index) +
            " (" + freetype_error_to_string(error) + ")"));
    }
}

void RendererStrategy::Text::Impl::render_glyph(
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

auto RendererStrategy::Text::Impl::font_path() -> std::string
{
    auto const path = default_font();
    if (path.empty())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to find a font"));
    }
    return path;
}

auto RendererStrategy::Text::Impl::utf8_to_utf32(std::string const& text) -> std::u32string
{
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    std::u32string utf32_text;
    try {
        utf32_text = converter.from_bytes(text);
    } catch(const std::range_error& e) {
        mir::log_warning("Window title %s is not valid UTF-8", text.c_str());
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

RendererStrategy::RendererStrategy(std::shared_ptr<StaticGeometry> const& static_geometry) :
    static_geometry{static_geometry},
    focused_theme{
        default_focused_background,
        default_focused_text},
    unfocused_theme{
        default_unfocused_background,
        default_unfocused_text},
    current_theme{nullptr},
    text{Text::instance()},
    button_icons{
        {msd::Button::Close, {
            default_close_normal_button,
            default_close_active_button,
            default_button_icon,
            render_close_icon}},
        {msd::Button::Maximize, {
            default_normal_button,
            default_active_button,
            default_button_icon,
            render_maximize_icon}},
        {msd::Button::Minimize, {
            default_normal_button,
            default_active_button,
            default_button_icon,
            render_minimize_icon}},
      }
{
}

void RendererStrategy::update_state(WindowState const& window_state, InputState const& input_state)
{
    if (auto const new_scale{window_state.scale()}; new_scale != scale)
    {
        scale = new_scale;

        needs_solid_color_redraw = true;
        solid_color_pixels.reset(); // force a reallocation next time it's needed

        needs_titlebar_redraw = true;
        titlebar_pixels.reset(); // force a reallocation next time it's needed

        needs_titlebar_buttons_redraw = true;
    }

    left_border_size = window_state.left_border_rect().size;
    right_border_size = window_state.right_border_rect().size;
    bottom_border_size = window_state.bottom_border_rect().size;

    size_t length{0};
    length = std::max(length, area(left_border_size));
    length = std::max(length, area(right_border_size));
    length = std::max(length, area(bottom_border_size));

    if (auto const scaled_length{static_cast<size_t>(length * scale * scale)};
        length != solid_color_pixels_length ||
        scaled_length != scaled_solid_color_pixels_length)
    {
        solid_color_pixels_length = length;
        scaled_solid_color_pixels_length = scaled_length;
        solid_color_pixels.reset(); // force a reallocation next time it's needed
    }

    if (window_state.titlebar_rect().size != titlebar_size)
    {
        titlebar_size = window_state.titlebar_rect().size;
        titlebar_pixels.reset(); // force a reallocation next time it's needed
    }

    auto const focus_state = window_state.focused_state();
    set_focus_state(focus_state);

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

auto RendererStrategy::render_titlebar(msd::BufferMaker const* maker) -> std::optional<std::shared_ptr<mir::graphics::Buffer>>
{
    auto const scaled_titlebar_size{titlebar_size * scale};

    if (!area(scaled_titlebar_size))
        return std::nullopt;

    if (!titlebar_pixels)
    {
        titlebar_pixels = alloc_pixels(scaled_titlebar_size);
        needs_titlebar_redraw = true;
    }

    if (needs_titlebar_redraw)
    {
        redraw_titlebar_background(scaled_titlebar_size);

        redraw_titlebar_text(scaled_titlebar_size);
    }

    if (needs_titlebar_redraw || needs_titlebar_buttons_redraw)
    {
        redraw_titlebar_buttons(scaled_titlebar_size);
    }

    needs_titlebar_redraw = false;
    needs_titlebar_buttons_redraw = false;

    return maker->make_buffer(static_geometry->buffer_format, scaled_titlebar_size, titlebar_pixels.get());
}

auto RendererStrategy::render_left_border(msd::BufferMaker const* maker) -> std::optional<std::shared_ptr<mir::graphics::Buffer>>
{
    auto const scaled_left_border_size{left_border_size * scale};
    if (!area(scaled_left_border_size))
        return std::nullopt;
    update_solid_color_pixels();
    return maker->make_buffer(static_geometry->buffer_format, scaled_left_border_size, solid_color_pixels.get());
}

auto RendererStrategy::render_right_border(msd::BufferMaker const* maker) -> std::optional<std::shared_ptr<mir::graphics::Buffer>>
{
    auto const scaled_right_border_size{right_border_size * scale};
    if (!area(scaled_right_border_size))
        return std::nullopt;
    update_solid_color_pixels();
    return maker->make_buffer(static_geometry->buffer_format, scaled_right_border_size, solid_color_pixels.get());
}

auto RendererStrategy::render_bottom_border(msd::BufferMaker const* maker) -> std::optional<std::shared_ptr<mir::graphics::Buffer>>
{
    auto const scaled_bottom_border_size{bottom_border_size * scale};
    if (!area(scaled_bottom_border_size))
        return std::nullopt;
    update_solid_color_pixels();
    return maker->make_buffer(static_geometry->buffer_format, scaled_bottom_border_size, solid_color_pixels.get());
}

void RendererStrategy::update_solid_color_pixels()
{
    if (!solid_color_pixels)
    {
        solid_color_pixels = alloc_pixels(geom::Size{scaled_solid_color_pixels_length, 1});
        needs_solid_color_redraw = true;
    }

    if (needs_solid_color_redraw)
    {
        render_row(
            solid_color_pixels.get(), geom::Size{scaled_solid_color_pixels_length, 1},
            geom::Point{}, geom::Width{scaled_solid_color_pixels_length},
            current_theme->background_color);
    }

    needs_solid_color_redraw = false;
}

void RendererStrategy::redraw_titlebar_background(geom::Size const scaled_titlebar_size)
{
    for (geom::Y y{0}; y < as_y(scaled_titlebar_size.height); y += geom::DeltaY{1})
    {
        render_row(
            titlebar_pixels.get(), scaled_titlebar_size,
            {0, y}, scaled_titlebar_size.width,
            current_theme->background_color);
    }
}

void RendererStrategy::redraw_titlebar_text(geom::Size const scaled_titlebar_size)
{
    text->render(
        titlebar_pixels.get(),
        scaled_titlebar_size,
        name,
        geom::Point{
            static_geometry->title_font_top_left.x.as_value() * scale,
            static_geometry->title_font_top_left.y.as_value() * scale},
        static_geometry->title_font_height * scale,
        current_theme->text_color);
}

void RendererStrategy::redraw_titlebar_buttons(geom::Size const scaled_titlebar_size)
{
    for (auto const& button : buttons)
    {
        geom::Rectangle scaled_button_rect{
            geom::Point{
                button.rect.left().as_value() * scale,
                button.rect.top().as_value() * scale},
            button.rect.size * scale};
        auto const icon = button_icons.find(button.function);
        if (icon != button_icons.end())
        {
            Pixel button_color = icon->second.normal_color;
            if (button.state == msd::Button::Hovered)
                button_color = icon->second.active_color;
            for (geom::Y y{scaled_button_rect.top()}; y < scaled_button_rect.bottom(); y += geom::DeltaY{1})
            {
                render_row(
                    titlebar_pixels.get(),
                    scaled_titlebar_size,
                    {scaled_button_rect.left(), y},
                    scaled_button_rect.size.width,
                    button_color);
            }
            geom::Rectangle const icon_rect = {
                scaled_button_rect.top_left + static_geometry->icon_padding * scale, {
                    scaled_button_rect.size.width - static_geometry->icon_padding.dx * scale * 2,
                    scaled_button_rect.size.height - static_geometry->icon_padding.dy * scale * 2}};
            icon->second.render_icon(
                titlebar_pixels.get(),
                scaled_titlebar_size,
                icon_rect,
                static_geometry->icon_line_width * scale,
                icon->second.icon_color);
        }
        else
        {
            mir::log_warning("Could not render decoration button with unknown function %d\n", static_cast<int>(button.function));
        }
    }
}

void RendererStrategy::set_focus_state(MirWindowFocusState focus_state)
{
    Theme const* const new_theme = (focus_state != mir_window_focus_state_unfocused) ?
                                       &focused_theme :
                                       &unfocused_theme;

    if (new_theme != current_theme)
    {
        current_theme = new_theme;
        needs_titlebar_redraw = true;
        needs_solid_color_redraw = true;
    }
}


auto DecorationStrategy::static_geometry() const -> std::shared_ptr<StaticGeometry>
{
    static auto const geometry{std::make_shared<StaticGeometry>(
        geom::Height{24},   // titlebar_height
        geom::Width{6},     // side_border_width
        geom::Height{6},    // bottom_border_height
        geom::Size{16, 16}, // resize_corner_input_size
        geom::Width{24},    // button_width
        geom::Width{6},     // padding_between_buttons
        geom::Height{14},   // title_font_height
        geom::Point{8, 2},  // title_font_top_left
        geom::Displacement{5, 5}, // icon_padding
        geom::Width{1},     // detail_line_width
        mir_pixel_format_argb_8888  // buffer_format
    )};
    return geometry;
}

auto DecorationStrategy::render_strategy() const -> std::unique_ptr<mir::shell::decoration::RendererStrategy>
{
    return std::make_unique<::RendererStrategy>(static_geometry());
}

auto DecorationStrategy::button_placement(unsigned n, const WindowState& ws) const -> geom::Rectangle
{
    auto const titlebar = ws.titlebar_rect();
    auto const geometry = static_geometry();

    geom::X x =
        titlebar.right() -
        as_delta(ws.side_border_width()) -
        n * as_delta(geometry->button_width + geometry->padding_between_buttons) -
        as_delta(geometry->button_width);
    return geom::Rectangle{
                    {x, titlebar.top()},
                    {geometry->button_width, titlebar.size.height}};
}
