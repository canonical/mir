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

#ifndef MIR_FRONTEND_TEXT_INPUT_V1_H
#define MIR_FRONTEND_TEXT_INPUT_V1_H

#include "text_input_unstable_v1.h"
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
struct TextInputV1Ctx;

class TextInputManagerV1
    : public wayland_rs::ZwpTextInputManagerV1Impl
{
public:
    TextInputManagerV1(
        std::shared_ptr<Executor> wayland_executor,
        std::shared_ptr<scene::TextInputHub> text_input_hub,
        std::shared_ptr<wayland_rs::Client> const& client);

    auto create_text_input() -> std::shared_ptr<wayland_rs::ZwpTextInputV1Impl> override;

private:
    std::shared_ptr<TextInputV1Ctx> ctx;
    std::shared_ptr<wayland_rs::Client> const client;
};
}
}

#endif //MIR_FRONTEND_TEXT_INPUT_V1_H
