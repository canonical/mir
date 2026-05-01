/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_TEXT_INPUT_V2_H
#define MIR_FRONTEND_TEXT_INPUT_V2_H

#include "text_input_unstable_v2.h"
#include "client.h"
#include "weak.h"

#include <memory>

namespace mir
{
class Executor;
namespace scene
{
class TextInputHub;
}
namespace frontend
{
struct TextInputV2Ctx;

class TextInputManagerV2
    : public wayland_rs::ZwpTextInputManagerV2Impl
{
public:
    TextInputManagerV2(
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<scene::TextInputHub> const& text_input_hub,
        std::shared_ptr<wayland_rs::Client> const& client);

    auto get_text_input(wayland_rs::Weak<wayland_rs::WlSeatImpl> const& seat)
        -> std::shared_ptr<wayland_rs::ZwpTextInputV2Impl> override;

private:
    std::shared_ptr<TextInputV2Ctx> const ctx;
    std::shared_ptr<wayland_rs::Client> const client;
};
}
}

#endif // MIR_FRONTEND_TEXT_INPUT_V2_H
