/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/geometry/rectangle.h"
#include "src/server/compositor/occlusion.h"
#include "examples/demo-shell/occlusion.h"
#include "mir_test_doubles/fake_renderable.h"
#include "mir_test_doubles/stub_scene_element.h"

#include <gtest/gtest.h>
#include <memory>

using namespace testing;
using namespace mir::geometry;
using namespace mir::compositor;
namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;

namespace
{
struct DefaultOcclusion
{
    SceneElementSequence filter(SceneElementSequence& list, Rectangle const& area)
    {
        return filter_occlusions_from(list, area);
    }
};

struct DecoratedOcclusion
{   
    SceneElementSequence filter(SceneElementSequence& list, Rectangle const& area)
    {
        return mir::examples::filter_occlusions_from(list, area, 0, 0);
    }
};

SceneElementSequence scene_elements_from(
    std::vector<std::shared_ptr<mg::Renderable>> renderables)
{
    SceneElementSequence elements;
    for (auto const& renderable : renderables)
        elements.push_back(std::make_shared<mtd::StubSceneElement>(renderable));

    return elements;
}

mg::RenderableList renderables_from(SceneElementSequence const& elements)
{
    mg::RenderableList renderables;
    for (auto const& element : elements)
        renderables.push_back(element->renderable());

    return renderables;
}

template<typename T>
SceneElementSequence filter(SceneElementSequence& list, Rectangle const& area)
{
    T filter;
    return filter.filter(list, area);
}

template<typename T>
struct OcclusionFilterTest : public Test
{
    OcclusionFilterTest()
    {
        monitor_rect.top_left = {0, 0};
        monitor_rect.size = {1920, 1200};
    }

    Rectangle monitor_rect;
};
typedef ::testing::Types<DefaultOcclusion, DecoratedOcclusion> OcclusionTypes;
TYPED_TEST_CASE(OcclusionFilterTest, OcclusionTypes);

}

TYPED_TEST(OcclusionFilterTest, single_window_not_occluded)
{
    auto window = std::make_shared<mtd::FakeRenderable>(12, 34, 56, 78);
    auto elements = scene_elements_from({window});

    auto const& occlusions = filter<TypeParam>(elements, this->monitor_rect);

    EXPECT_THAT(renderables_from(occlusions), IsEmpty());
    EXPECT_THAT(renderables_from(elements), ElementsAre(window));
}

TYPED_TEST(OcclusionFilterTest, partially_offscreen_still_visible)
{ // Regression test for LP: #1301115
    auto left =   std::make_shared<mtd::FakeRenderable>(-10,   10, 100, 100);
    auto right =  std::make_shared<mtd::FakeRenderable>(1900,  10, 100, 100);
    auto top =    std::make_shared<mtd::FakeRenderable>(500,   -1, 100, 100);
    auto bottom = std::make_shared<mtd::FakeRenderable>(200, 1000, 100, 1000);
    auto elements = scene_elements_from({left, right, top, bottom});
 
    auto const& occlusions = filter<TypeParam>(elements, this->monitor_rect);

    EXPECT_THAT(renderables_from(occlusions), IsEmpty());
    EXPECT_THAT(renderables_from(elements), ElementsAre(left, right, top, bottom));
}

TYPED_TEST(OcclusionFilterTest, smaller_window_occluded)
{
    auto top = std::make_shared<mtd::FakeRenderable>(10, 10, 10, 10);
    auto bottom = std::make_shared<mtd::FakeRenderable>(12, 12, 5, 5);
    auto elements = scene_elements_from({bottom, top});

    auto const& occlusions = filter<TypeParam>(elements, this->monitor_rect);

    EXPECT_THAT(renderables_from(occlusions), ElementsAre(bottom));
    EXPECT_THAT(renderables_from(elements), ElementsAre(top));
}

TYPED_TEST(OcclusionFilterTest, translucent_window_occludes_nothing)
{
    auto top = std::make_shared<mtd::FakeRenderable>(Rectangle{{10, 10}, {10, 10}}, 0.5f);
    auto bottom = std::make_shared<mtd::FakeRenderable>(Rectangle{{12, 12}, {5, 5}}, 1.0f);
    auto elements = scene_elements_from({bottom, top});

    auto const& occlusions = filter<TypeParam>(elements, this->monitor_rect);

    EXPECT_THAT(renderables_from(occlusions), IsEmpty());
    EXPECT_THAT(renderables_from(elements), ElementsAre(bottom, top));
}

TYPED_TEST(OcclusionFilterTest, hidden_window_is_self_occluded)
{
    auto window = std::make_shared<mtd::FakeRenderable>(Rectangle{{10, 10}, {10, 10}}, 1.0f, true, false);
    auto elements = scene_elements_from({window});

    auto const& occlusions = filter<TypeParam>(elements, this->monitor_rect);

    EXPECT_THAT(renderables_from(occlusions), ElementsAre(window));
    EXPECT_THAT(renderables_from(elements), IsEmpty());
}

TYPED_TEST(OcclusionFilterTest, hidden_window_occludes_nothing)
{
    auto top = std::make_shared<mtd::FakeRenderable>(Rectangle{{10, 10}, {10, 10}}, 1.0f, true, false);
    auto bottom = std::make_shared<mtd::FakeRenderable>(12, 12, 5, 5);
    auto elements = scene_elements_from({bottom, top});

    auto const& occlusions = filter<TypeParam>(elements, this->monitor_rect);

    EXPECT_THAT(renderables_from(occlusions), ElementsAre(top));
    EXPECT_THAT(renderables_from(elements), ElementsAre(bottom));
}

TYPED_TEST(OcclusionFilterTest, shaped_window_occludes_nothing)
{
    auto top = std::make_shared<mtd::FakeRenderable>(Rectangle{{10, 10}, {10, 10}}, 1.0f, false, true);
    auto bottom = std::make_shared<mtd::FakeRenderable>(12, 12, 5, 5);
    auto elements = scene_elements_from({bottom, top});

    auto const& occlusions = filter<TypeParam>(elements, this->monitor_rect);

    EXPECT_THAT(renderables_from(occlusions), IsEmpty());
    EXPECT_THAT(renderables_from(elements), ElementsAre(bottom, top));
}

TYPED_TEST(OcclusionFilterTest, identical_window_occluded)
{
    auto top = std::make_shared<mtd::FakeRenderable>(10, 10, 10, 10);
    auto bottom = std::make_shared<mtd::FakeRenderable>(10, 10, 10, 10);
    auto elements = scene_elements_from({bottom, top});

    auto const& occlusions = filter<TypeParam>(elements, this->monitor_rect);

    EXPECT_THAT(renderables_from(occlusions), ElementsAre(bottom));
    EXPECT_THAT(renderables_from(elements), ElementsAre(top));
}

TYPED_TEST(OcclusionFilterTest, larger_window_never_occluded)
{
    auto top = std::make_shared<mtd::FakeRenderable>(10, 10, 10, 10);
    auto bottom = std::make_shared<mtd::FakeRenderable>(9, 9, 12, 12);
    auto elements = scene_elements_from({bottom, top});

    auto const& occlusions = filter<TypeParam>(elements, this->monitor_rect);

    EXPECT_THAT(renderables_from(occlusions), IsEmpty());
    EXPECT_THAT(renderables_from(elements), ElementsAre(bottom, top));
}

TYPED_TEST(OcclusionFilterTest, cascaded_windows_never_occluded)
{
    std::vector<std::shared_ptr<mg::Renderable>> renderables;
    unsigned int const num_windows{10u};
    for (auto x = 0u; x < num_windows; x++)
        renderables.push_back(std::make_shared<mtd::FakeRenderable>(x, x, 200, 100));

    auto elements = scene_elements_from(renderables);

    auto const& occlusions = filter<TypeParam>(elements, this->monitor_rect);

    EXPECT_THAT(renderables_from(occlusions), IsEmpty());
    EXPECT_THAT(renderables_from(elements), ElementsAreArray(renderables));
}

TYPED_TEST(OcclusionFilterTest, some_occluded_and_some_not)
{
    auto window0 = std::make_shared<mtd::FakeRenderable>(10, 20, 400, 300);
    auto window1 = std::make_shared<mtd::FakeRenderable>(10, 20, 5, 5);
    auto window2 = std::make_shared<mtd::FakeRenderable>(100, 100, 20, 20);
    auto window3 = std::make_shared<mtd::FakeRenderable>(200, 200, 50, 50);
    auto window4 = std::make_shared<mtd::FakeRenderable>(500, 600, 34, 56);
    auto window5 = std::make_shared<mtd::FakeRenderable>(200, 200, 1000, 1000);
    auto elements = scene_elements_from({
        window5, //not occluded
        window4, //not occluded
        window3, //occluded
        window2, //occluded
        window1, //occluded
        window0  //not occluded
    });

    auto const& occlusions = filter<TypeParam>(elements, this->monitor_rect);

    EXPECT_THAT(renderables_from(occlusions), ElementsAre(window3, window2, window1));
    EXPECT_THAT(renderables_from(elements), ElementsAre(window5, window4, window0));
}

TYPED_TEST(OcclusionFilterTest,
       occludes_partially_onscreen_window_when_onscreen_part_is_covered_by_another_window)
{
    auto const partially_onscreen = std::make_shared<mtd::FakeRenderable>(-50, 100, 150, 100);
    auto const covering = std::make_shared<mtd::FakeRenderable>(0, 100, 100, 100);
    auto elements = scene_elements_from({
        partially_onscreen,
        covering
    });

    auto const& occlusions = filter<TypeParam>(elements, this->monitor_rect);

    EXPECT_THAT(renderables_from(occlusions), ElementsAre(partially_onscreen));
    EXPECT_THAT(renderables_from(elements), ElementsAre(covering));
}

//example compositor
TEST(DecoratedOcclusionFilter, accounts_for_decorations_while_filtering) //lp: 1299977
{
    Rectangle monitor_rect{{0,0},{100, 100}};
    auto window0 = std::make_shared<mtd::FakeRenderable>(0, 100, 10, 10);
    auto window1 = std::make_shared<mtd::FakeRenderable>(-5, -5, 5, 5);
    auto window2 = std::make_shared<mtd::FakeRenderable>(0, 30, 1, 1);
    auto window3 = std::make_shared<mtd::FakeRenderable>(0, 30, 15, 15);
    auto elements = scene_elements_from({
        window0, //surface offscreen, but titlebar should still appear
        window1, //surface offscreen, but shadow should still appear
        window2, //occluded
        window3, //not occluded
    });

    auto const& occlusions = mir::examples::filter_occlusions_from(elements, monitor_rect, 10, 20);

    EXPECT_THAT(renderables_from(occlusions), ElementsAre(window2));
    EXPECT_THAT(renderables_from(elements), ElementsAre(window0, window1, window3));
}
