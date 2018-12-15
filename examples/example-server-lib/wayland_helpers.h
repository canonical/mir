/*
 * Copyright © 2018 Canonical Ltd.
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

#ifndef MIRAL_WAYLAND_HELPERS_H
#define MIRAL_WAYLAND_HELPERS_H

#include <wayland-client.h>
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>

template<typename Type>
auto make_scoped(Type* owned, void(*deleter)(Type*)) -> std::unique_ptr<Type, void(*)(Type*)>
{
    return {owned, deleter};
}

wl_shm_pool* make_shm_pool(struct wl_shm* shm, int size, void **data);

class Output
{
public:
    Output(
        wl_output* output,
        std::function<void(Output const&)> on_constructed,
        std::function<void(Output const&)> on_change);
    ~Output();

    Output(Output const&) = delete;
    Output(Output&&) = delete;

    Output& operator=(Output const&) = delete;
    Output& operator=(Output&&) = delete;

    int32_t x, y;
    int32_t width, height;
    int32_t transform;
    wl_output* output;
private:
    static void output_done(void* data, wl_output* output);

    static wl_output_listener const output_listener;

    std::function<void(Output const&)> on_constructed;
    std::function<void(Output const&)> on_change;
};

class Globals
{
public:
    Globals(
        std::function<void(Output const&)> on_new_output,
        std::function<void(Output const&)> on_output_changed,
        std::function<void(Output const&)> on_output_gone);

    wl_compositor* compositor = nullptr;
    wl_shm* shm = nullptr;
    wl_seat* seat = nullptr;
    wl_shell* shell = nullptr;

    void init(struct wl_display* display);
    void teardown();

private:
    static void new_global(
        void* data,
        struct wl_registry* registry,
        uint32_t id,
        char const* interface,
        uint32_t version);

    static void global_remove(
        void* data,
        struct wl_registry* registry,
        uint32_t name);

    std::unordered_map<uint32_t, std::unique_ptr<Output>> bound_outputs;

    std::function<void(Output const&)> const on_new_output;
    std::function<void(Output const&)> const on_output_changed;
    std::function<void(Output const&)> const on_output_gone;
};

#endif //MIRAL_WAYLAND_HELPERS_H
