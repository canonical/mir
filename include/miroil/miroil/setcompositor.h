/*
 * Copyright © 2021 Canonical Ltd.
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
 * Authored by: Erlend <me@erlend.io>
 */

#ifndef MIROIL_SET_COMPOSITOR_H
#define MIROIL_SET_COMPOSITOR_H
#include <memory>
#include <functional>

namespace mir { class Server; }
namespace mir { namespace graphics { class Display; } }
namespace mir { namespace compositor { class DisplayListener; } } 

namespace miroil
{
    class Compositor;
       
// Configure the server for using the Qt compositor
class SetCompositor
{
    using initFunction = std::function<void(const std::shared_ptr<mir::graphics::Display>& display,
                       const std::shared_ptr<miroil::Compositor> & compositor,
                       const std::shared_ptr<mir::compositor::DisplayListener>& displayListener)>;
                       
    using constructorFunction = std::function<std::shared_ptr<miroil::Compositor>()>;    
    
public:
    SetCompositor(constructorFunction constructor, initFunction init);
    
    void operator()(mir::Server& server);

private:
    struct QtCompositorImpl;
    
    std::weak_ptr<QtCompositorImpl> m_compositor;
    constructorFunction m_constructor;    
    initFunction m_init;    
};

}

#endif //MIROIL_SET_COMPOSITOR_H
