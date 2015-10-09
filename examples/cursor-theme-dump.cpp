/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "xcursor_loader.h"

#include "mir/graphics/cursor_image.h"
#include <mir_toolkit/cursors.h>

#include <fstream>

using namespace mir::examples;
using namespace mir::geometry;

namespace
{
auto const cursor_names = {
    mir_arrow_cursor_name,
    mir_busy_cursor_name,
    mir_caret_cursor_name,
    mir_default_cursor_name,
    mir_pointing_hand_cursor_name,
    mir_open_hand_cursor_name,
    mir_closed_hand_cursor_name,
    mir_horizontal_resize_cursor_name,
    mir_vertical_resize_cursor_name,
    mir_diagonal_resize_bottom_to_top_cursor_name,
    mir_diagonal_resize_top_to_bottom_cursor_name,
    mir_omnidirectional_resize_cursor_name,
    mir_vsplit_resize_cursor_name,
    mir_hsplit_resize_cursor_name,
    mir_crosshair_cursor_name };
}


int main(int argc, char const* argv[])
try
{
    if (argc != 2)
    {
        puts("Lousy params");
        exit(-1);
    }

    auto const theme = argv[1];
    std::ofstream output(std::string{theme} + "-theme.h");

    output << "#include <initializer_list>\n"
      "\n"
      "namespace\n"
      "{\n"
        "struct CursorData\n"
        "{\n"
        "    CursorData(char const* name, unsigned int hotspot_x, unsigned int hotspot_y, char const* pixel_data) :\n"
        "        name(name), hotspot_x(hotspot_x), hotspot_y(hotspot_y), pixel_data(reinterpret_cast<unsigned char const*>(pixel_data)) {}\n"
        "\n"
        "    unsigned int const width{" << mir::input::default_cursor_size.width.as_int() << "};\n"
        "    unsigned int const height{" << mir::input::default_cursor_size.height.as_int() << "};\n"
        "    char const* const name;\n"
        "    unsigned int const hotspot_x;\n"
        "    unsigned int const hotspot_y;\n"
        "    unsigned char const* const pixel_data;\n"
        "};\n"
        "auto const cursor_data = {\n";

    auto const buffer_size = 4*mir::input::default_cursor_size.height.as_int()*mir::input::default_cursor_size.height.as_int();

    auto const xcursor_loader = std::make_shared<XCursorLoader>(theme);

    for (auto cursor : cursor_names)
    {
        if (auto const image = xcursor_loader->image(cursor, mir::input::default_cursor_size))
        {
            printf("Have image for %s:%s\n", theme, cursor);

            auto const hotspot = image->hotspot();
            auto const argb_8888 = static_cast<uint8_t const*>(image->as_argb_8888());
            auto const size = image->size();

            printf(" . . hotspot %d, %d\n", hotspot.dx.as_int(), hotspot.dy.as_int());
            printf(" . . size    %d, %d\n", size.width.as_int(), size.height.as_int());
            printf(" . . data    %p\n", argb_8888);

            output << "CursorData{\"" << cursor << "\", " << hotspot.dx.as_int() << ", " << hotspot.dy.as_int() << ",\n";

            int chars = 0;

            output << std::oct << "    \"";

            for (auto pbyte = argb_8888; pbyte != argb_8888 + buffer_size; ++pbyte)
            {
                auto step = (*pbyte < 010) ? 2 : (*pbyte < 0100) ? 3 : 4;

                if ((chars += step) > 80)
                {
                    output << "\"\n    \"";
                    chars = step;
                }

                output << '\\' << static_cast<unsigned>(*pbyte);
            }

            output << "\"\n";

            output << "},\n";


        }
        else
        {
            printf("No image for %s:%s\n", theme, cursor);
        }
    }
    output << "};\n";
    output << "}\n";
}
catch (std::exception const& error)
{
    printf("Error: %s", error.what());
}