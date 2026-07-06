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

#include <mir/shell/decoration/render_title_text.h>

#include <mir/default_font.h>
#include <mir/geometry/displacement.h>
#include <mir/log.h>

#include <boost/throw_exception.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <codecvt>
#include <locale>
#include <memory>
#include <mutex>
#include <string>

namespace geom = mir::geometry;
namespace msd = mir::shell::decoration;

namespace
{
constexpr auto pack_pixel(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 0xFF) -> uint32_t
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return (uint32_t(b) << 0) | (uint32_t(g) << 8) | (uint32_t(r) << 16) | (uint32_t(a) << 24);
#elif __BYTE_ORDER == __BIG_ENDIAN
    return (uint32_t(b) << 24) | (uint32_t(g) << 16) | (uint32_t(r) << 8) | (uint32_t(a) << 0);
#else
  #error unsupported byte order
#endif
}

inline auto area(geom::Size size) -> size_t
{
    return (size.width > geom::Width{} && size.height > geom::Height{})
        ? size.width.as_int() * size.height.as_int()
        : 0;
}

constexpr auto unpack_pixel(uint32_t c, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) -> void
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    b = static_cast<unsigned char>((c >> 0) & 0xFF);
    g = static_cast<unsigned char>((c >> 8) & 0xFF);
    r = static_cast<unsigned char>((c >> 16) & 0xFF);
    a = static_cast<unsigned char>((c >> 24) & 0xFF);
#elif __BYTE_ORDER == __BIG_ENDIAN
    b = static_cast<unsigned char>((c >> 24) & 0xFF);
    g = static_cast<unsigned char>((c >> 16) & 0xFF);
    r = static_cast<unsigned char>((c >> 8) & 0xFF);
    a = static_cast<unsigned char>((c >> 0) & 0xFF);
#else
  #error unsupported byte order
#endif
}

constexpr auto blend_pixel(uint32_t c, unsigned char r, unsigned char g, unsigned char b, unsigned char a) -> uint32_t
{
    unsigned char src_r = 0, src_g = 0, src_b = 0, src_a = 0;
    unpack_pixel(c, src_r, src_g, src_b, src_a);
    unsigned char const out_r =
        static_cast<unsigned char>((static_cast<int>(src_r) * (255 - a)) / 255 + (static_cast<int>(r) * a) / 255);
    unsigned char const out_g =
        static_cast<unsigned char>((static_cast<int>(src_g) * (255 - a)) / 255 + (static_cast<int>(g) * a) / 255);
    unsigned char const out_b =
        static_cast<unsigned char>((static_cast<int>(src_b) * (255 - a)) / 255 + (static_cast<int>(b) * a) / 255);
    unsigned char const out_a = src_a;
    return pack_pixel(out_r, out_g, out_b, out_a);
}

class TitleTextRenderer
{
public:
    static auto instance() -> std::shared_ptr<TitleTextRenderer>;

    virtual ~TitleTextRenderer() = default;

    virtual void render(
        msd::Pixel* buf,
        geom::Size buf_size,
        std::string const& text,
        geom::Point top_left,
        geom::Height height_pixels,
        msd::Pixel color) = 0;

private:
    class Impl;
    class Null;

    static std::mutex static_mutex;
    static std::weak_ptr<TitleTextRenderer> singleton;
};

class TitleTextRenderer::Impl : public TitleTextRenderer
{
public:
    Impl();
    ~Impl() override;

    void render(
        msd::Pixel* buf,
        geom::Size buf_size,
        std::string const& text,
        geom::Point top_left,
        geom::Height height_pixels,
        msd::Pixel color) override;

private:
    std::mutex mutex;
    FT_Library library{};
    FT_Face face{};

    void set_char_size(geom::Height height);
    void rasterize_glyph(char32_t glyph);
    void render_glyph(
        msd::Pixel* buf,
        geom::Size buf_size,
        FT_Bitmap const* glyph,
        geom::Point top_left,
        msd::Pixel color);

    static auto font_path() -> std::string;
    static auto utf8_to_utf32(std::string const& text) -> std::u32string;
};

class TitleTextRenderer::Null : public TitleTextRenderer
{
public:
    void render(msd::Pixel*, geom::Size, std::string const&, geom::Point, geom::Height, msd::Pixel) override {}
};

std::mutex TitleTextRenderer::static_mutex;
std::weak_ptr<TitleTextRenderer> TitleTextRenderer::singleton;

auto TitleTextRenderer::instance() -> std::shared_ptr<TitleTextRenderer>
{
    std::lock_guard const lock{static_mutex};
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

TitleTextRenderer::Impl::Impl()
{
    if (auto const error = FT_Init_FreeType(&library))
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Initializing freetype library failed with error " + std::to_string(error)));

    auto const path = font_path();
    if (auto const error = FT_New_Face(library, path.c_str(), 0, &face))
    {
        if (error == FT_Err_Unknown_File_Format)
            BOOST_THROW_EXCEPTION(std::runtime_error("Font " + path + " has unsupported format"));
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Loading font from " + path + " failed with error " + std::to_string(error)));
    }
}

TitleTextRenderer::Impl::~Impl()
{
    if (auto const error = FT_Done_Face(face))
        mir::log_warning("Failed to uninitialize font face with error %d", error);
    face = nullptr;

    if (auto const error = FT_Done_FreeType(library))
        mir::log_warning("Failed to uninitialize FreeType with error %d", error);
    library = nullptr;
}

void TitleTextRenderer::Impl::render(
    msd::Pixel* buf,
    geom::Size buf_size,
    std::string const& text,
    geom::Point top_left,
    geom::Height height_pixels,
    msd::Pixel color)
{
    if (!area(buf_size) || height_pixels <= geom::Height{})
        return;

    std::lock_guard const lock{mutex};

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

            geom::Point const glyph_top_left =
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

void TitleTextRenderer::Impl::set_char_size(geom::Height height)
{
    if (auto const error = FT_Set_Pixel_Sizes(face, 0, height.as_int()))
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Setting char size failed with error " + std::to_string(error)));
}

auto freetype_error_to_string(FT_Error error) -> char const*
{
    auto const reason = FT_Error_String(error);
    return reason ? reason : "<unknown error>";
}

void TitleTextRenderer::Impl::rasterize_glyph(char32_t glyph)
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

void TitleTextRenderer::Impl::render_glyph(
    msd::Pixel* buf,
    geom::Size buf_size,
    FT_Bitmap const* glyph,
    geom::Point top_left,
    msd::Pixel color)
{
    geom::X const buffer_left = std::max(top_left.x, geom::X{});
    geom::X const buffer_right = std::min(top_left.x + geom::DeltaX{glyph->width}, mir::geometry::as_x(buf_size.width));

    geom::Y const buffer_top = std::max(top_left.y, geom::Y{});
    geom::Y const buffer_bottom =
        std::min(top_left.y + geom::DeltaY{glyph->rows}, mir::geometry::as_y(buf_size.height));

    geom::Displacement const glyph_offset = as_displacement(top_left);

    unsigned char color_red = 0, color_green = 0, color_blue = 0, color_alpha = 0;
    unpack_pixel(color, color_red, color_green, color_blue, color_alpha);

    for (geom::Y buffer_y = buffer_top; buffer_y < buffer_bottom; buffer_y += geom::DeltaY{1})
    {
        geom::Y const glyph_y = buffer_y - glyph_offset.dy;
        unsigned char* const glyph_row = glyph->buffer + glyph_y.as_int() * glyph->pitch;
        msd::Pixel* const buffer_row = buf + buffer_y.as_int() * buf_size.width.as_int();

        for (geom::X buffer_x = buffer_left; buffer_x < buffer_right; buffer_x += geom::DeltaX{1})
        {
            geom::X const glyph_x = buffer_x - glyph_offset.dx;
            unsigned char const glyph_alpha =
                static_cast<unsigned char>((static_cast<int>(glyph_row[glyph_x.as_int()]) * color_alpha) / 255);
            buffer_row[buffer_x.as_int()] =
                blend_pixel(buffer_row[buffer_x.as_int()], color_red, color_green, color_blue, glyph_alpha);
        }
    }
}

auto TitleTextRenderer::Impl::font_path() -> std::string
{
    auto const path = mir::default_font();
    if (path.empty())
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to find a font"));
    return path;
}

auto TitleTextRenderer::Impl::utf8_to_utf32(std::string const& text) -> std::u32string
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
#pragma GCC diagnostic pop

    std::u32string utf32_text;
    try
    {
        utf32_text = converter.from_bytes(text);
    }
    catch (std::range_error const&)
    {
        mir::log_warning("Window title %s is not valid UTF-8", text.c_str());
        for (char const c : text)
            utf32_text += isprint(c) ? static_cast<char32_t>(c) : static_cast<char32_t>(0xFFFD);
    }
    return utf32_text;
}

} // namespace

void msd::render_title_text(
    Pixel* buf,
    geometry::Size buf_size,
    std::string const& text,
    geometry::Point top_left,
    geometry::Height height_pixels,
    Pixel color)
{
    if (auto const title = TitleTextRenderer::instance())
        title->render(buf, buf_size, text, top_left, height_pixels, color);
}
