/*
 * Copyright © 2016 Canonical Ltd.
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

#ifndef MIRAL_COORDINATE_TRANSLATOR_H
#define MIRAL_COORDINATE_TRANSLATOR_H

#include <mir/scene/coordinate_translator.h>

#include <atomic>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif

namespace miral
{
class CoordinateTranslator : public mir::scene::CoordinateTranslator
{
public:
    void enable_debug_api();
    void disable_debug_api();

    auto surface_to_screen(std::shared_ptr<mir::frontend::Surface> surface, int32_t x, int32_t y)
        -> mir::geometry::Point override;

    bool translation_supported() const;

private:
    std::atomic<bool> enabled{false};
};
}
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif //MIRAL_COORDINATE_TRANSLATOR_H
