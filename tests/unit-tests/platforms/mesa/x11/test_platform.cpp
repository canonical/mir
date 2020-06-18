/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mir/options/program_option.h"
#include "src/platforms/x11/graphics/platform.h"
#include "src/server/report/null/display_report.h"

#include "mir/test/doubles/mock_x11.h"
#include "mir/shared_library.h"
#include "mir_test_framework/executable_path.h"

namespace mir
{
namespace graphics
{
namespace X
{
auto operator==(X11OutputConfig const& a, X11OutputConfig const& b) -> bool
{
    return a.size == b.size &&
           testing::Value(a.scale, testing::FloatEq(b.scale));
}

auto operator<<(std::ostream& os, X11OutputConfig const& config) -> std::ostream&
{
    return os << "size: " << config.size << ", scale: " << config.scale;
}
}
}
}

namespace mg = mir::graphics;
namespace mgx = mir::graphics::X;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{
const char probe_platform[] = "probe_graphics_platform";

class X11GraphicsPlatformTest : public ::testing::Test
{
public:
    void SetUp()
    {
    }

    std::shared_ptr<mg::Platform> create_platform()
    {
        return std::make_shared<mg::X::Platform>(
            std::shared_ptr<::Display>(
                XOpenDisplay(nullptr),
                [](::Display* display)
                {
                    XCloseDisplay(display);
                }),
            std::vector<mg::X::X11OutputConfig>{{{1280, 1024}}},
            std::make_shared<mir::report::null::DisplayReport>());
    }

    ::testing::NiceMock<mtd::MockX11> mock_x11;
};
}

TEST_F(X11GraphicsPlatformTest, failure_to_open_x11_display_results_in_an_error)
{
    using namespace ::testing;

    EXPECT_CALL(mock_x11, XOpenDisplay(_))
        .WillRepeatedly(Return(nullptr));

    EXPECT_THROW({ create_platform(); }, std::exception);
}

TEST_F(X11GraphicsPlatformTest, probe_returns_unsupported_when_x_cannot_open_display)
{
    using namespace ::testing;

    mir::options::ProgramOption options;

    EXPECT_CALL(mock_x11, XOpenDisplay(_))
        .WillRepeatedly(Return(nullptr));

    mir::SharedLibrary platform_lib{mtf::server_platform("server-mesa-x11")};
    auto probe = platform_lib.load_function<mg::PlatformProbe>(probe_platform);
    EXPECT_EQ(mg::PlatformPriority::unsupported, probe(nullptr, options));
}

TEST_F(X11GraphicsPlatformTest, parses_simple_output_size)
{
    using namespace ::testing;

    auto str = "1280x720";
    auto parsed = mg::X::Platform::parse_output_sizes(str);

    EXPECT_THAT(parsed, ElementsAre(mgx::X11OutputConfig{{1280, 720}}));
}

TEST_F(X11GraphicsPlatformTest, parses_output_size_with_whole_number_scale)
{
    using namespace ::testing;

    auto str = "1280x720^2";
    auto parsed = mg::X::Platform::parse_output_sizes(str);

    EXPECT_THAT(parsed, ElementsAre(mgx::X11OutputConfig{{1280, 720}, 2}));
}

TEST_F(X11GraphicsPlatformTest, parses_output_size_with_fractional_scale)
{
    using namespace ::testing;

    auto str = "1280x720^0.8";
    auto parsed = mg::X::Platform::parse_output_sizes(str);

    // X11OutputConfig equality operator does not do exact float comparison
    EXPECT_THAT(parsed, ElementsAre(mgx::X11OutputConfig{{1280, 720}, 0.8}));
}

TEST_F(X11GraphicsPlatformTest, parses_multiple_output_sizes)
{
    using namespace ::testing;

    auto str = "1280x1024:600x600:30x750";
    auto parsed = mg::X::Platform::parse_output_sizes(str);

    EXPECT_THAT(parsed, ElementsAre(
        mgx::X11OutputConfig{{1280, 1024}},
        mgx::X11OutputConfig{{600, 600}},
        mgx::X11OutputConfig{{30, 750}}));
}

TEST_F(X11GraphicsPlatformTest, parses_multiple_output_sizes_some_with_scales)
{
    using namespace ::testing;

    auto str = "1280x1024^.9:600x600:30x750^12";
    auto parsed = mg::X::Platform::parse_output_sizes(str);

    EXPECT_THAT(parsed, ElementsAre(
        mgx::X11OutputConfig{{1280, 1024}, 0.9},
        mgx::X11OutputConfig{{600, 600}},
        mgx::X11OutputConfig{{30, 750}, 12}));
}

TEST_F(X11GraphicsPlatformTest, output_size_parsing_throws_on_bad_input)
{
    using namespace ::testing;
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("1280"), std::runtime_error) << "No height or 'x'";
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("1280x"), std::runtime_error) << "No height";
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("x1280"), std::runtime_error) << "No width";
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("=20x40"), std::runtime_error) << "Random equals";
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("20x30x40"), std::runtime_error) << "Too many dimensions";
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("1280x720:"), std::runtime_error) << "Ends with delim";
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("1280.5x720"), std::runtime_error) << "Fractional size";
    EXPECT_THROW(mg::X::Platform::parse_output_sizes(":1280x720"), std::runtime_error) << "Starts with delim";
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("1280x720:500"), std::runtime_error) << "No height or x on 2nd size";
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("50x50:x20:50x50"), std::runtime_error) << "No width on 2nd size";
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("1280x1024,1280x1024"), std::runtime_error) << "Uses comma delim";
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("0x0"), std::runtime_error) << "Zero size";
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("0x200"), std::runtime_error) << "Zero width";
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("200x-300"), std::runtime_error) << "Negative height";
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("200x300^"), std::runtime_error) << "Ends with ^";
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("200x^2"), std::runtime_error) << "Scale but no height";
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("200x300^2^2"), std::runtime_error) << "Multiple scales";
    EXPECT_THROW(mg::X::Platform::parse_output_sizes("200x300^2..8"), std::runtime_error) << "Multiple scale decimal points";
}
