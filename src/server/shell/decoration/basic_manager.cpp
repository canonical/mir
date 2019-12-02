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

#include "basic_manager.h"
#include "decoration.h"

#include <boost/throw_exception.hpp>
#include <vector>

namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace msh = mir::shell;
namespace msd = mir::shell::decoration;

msd::BasicManager::BasicManager(DecorationBuilder&& decoration_builder)
    : decoration_builder{decoration_builder}
{
}

msd::BasicManager::~BasicManager()
{
    undecorate_all();
}

void msd::BasicManager::init(std::weak_ptr<shell::Shell> const& shell_)
{
    shell = shell_;
}

void msd::BasicManager::decorate(std::shared_ptr<ms::Surface> const& surface)
{
    auto const locked_shell = shell.lock();
    if (!locked_shell)
        BOOST_THROW_EXCEPTION(std::runtime_error("Shell is null"));

    std::unique_lock<std::mutex> lock{mutex};
    if (decorations.find(surface.get()) == decorations.end())
    {
        decorations[surface.get()] = nullptr;
        lock.unlock();
        auto decoration = decoration_builder(locked_shell, surface);
        lock.lock();
        decorations[surface.get()] = std::move(decoration);
    }
}

void msd::BasicManager::undecorate(std::shared_ptr<ms::Surface> const& surface)
{
    std::unique_ptr<Decoration> decoration;
    {
        std::lock_guard<std::mutex> lock{mutex};
        auto const it = decorations.find(surface.get());
        if (it != decorations.end())
        {
            decoration = std::move(it->second);
            decorations.erase(it);
        }
    }
    // Destroy the decoration outside the lock
    decoration.reset();
}

void msd::BasicManager::undecorate_all()
{
    std::vector<std::unique_ptr<Decoration>> to_destroy;
    {
        std::lock_guard<std::mutex> lock{mutex};
        for (auto& it : decorations)
            to_destroy.push_back(std::move(it.second));
        decorations.clear();
    }
    // Destroy the decorations outside the lock
    to_destroy.clear();
}
