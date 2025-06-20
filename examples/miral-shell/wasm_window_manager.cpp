/*
 * Copyright © Canonical Ltd.
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
 */

#include "wasm_window_manager.h"

#include <wasmer.h>

using namespace miral;

namespace
{

}

class experimental::WasmWindowManager::Self
{
public:
    explicit Self(WindowManagerTools const& tools)
        : tools(tools),
          engine(wasm_engine_new()),
          store(wasm_store_new(engine))
    {
        const char *wat_string =
        "(module\n"
        "  (type $sum_t (func (param i32 i32) (result i32)))\n"
        "  (func $sum_f (type $sum_t) (param $x i32) (param $y i32) (result i32)\n"
        "    local.get $x\n"
        "    local.get $y\n"
        "    i32.add)\n"
        "  (export \"sum\" (func $sum_f)))";

        wasm_byte_vec_t wat;
        wasm_byte_vec_new(&wat, strlen(wat_string), wat_string);
        wasm_byte_vec_t wasm_bytes;
        wat2wasm(&wat, &wasm_bytes);
        wasm_byte_vec_delete(&wat);

        module = wasm_module_new(store, &wasm_bytes);

        wasm_byte_vec_delete(&wasm_bytes);
        wasm_extern_vec_t import_object = WASM_EMPTY_VEC;
        instance = wasm_instance_new(store, module, &import_object, NULL);

    }

    ~Self()
    {
        wasm_module_delete(module);
        wasm_instance_delete(instance);
        wasm_store_delete(store);
        wasm_engine_delete(engine);
    }

    int sum(int x, int y)
    {
        wasm_extern_vec_t exports;
        wasm_instance_exports(instance, &exports);
        wasm_func_t* sum_func = wasm_extern_as_func(exports.data[0]);
        if (sum_func == NULL) {
            printf("> Failed to get the `sum` function!\n");
            return 1;
        }

        printf("Calling `sum` function...\n");
        wasm_val_t args_val[2] = { WASM_I32_VAL(x), WASM_I32_VAL(y) };
        wasm_val_t results_val[1] = { WASM_INIT_VAL };
        wasm_val_vec_t args = WASM_ARRAY_VEC(args_val);
        wasm_val_vec_t results = WASM_ARRAY_VEC(results_val);

        if (wasm_func_call(sum_func, &args, &results)) {
            printf("> Error calling the `sum` function!\n");
        }

        wasm_extern_vec_delete(&exports);
        printf("Results of `sum`: %d\n", results_val[0].of.i32);
        return results_val[0].of.i32;
    }

private:
    WindowManagerTools tools;
    wasm_engine_t* engine;
    wasm_store_t* store;
    wasm_module_t* module;
    wasm_instance_t* instance;
};

experimental::WasmWindowManager::WasmWindowManager(WindowManagerTools const& tools)
    : self(std::make_shared<Self>(tools))
{
    self->sum(10, 15);
}

experimental::WasmWindowManager::~WasmWindowManager()
{

}

auto experimental::WasmWindowManager::place_new_window(
    ApplicationInfo const&, WindowSpecification const& requested_specification) -> WindowSpecification
{
    return requested_specification;
}

void experimental::WasmWindowManager::handle_window_ready(WindowInfo&)
{

}

void experimental::WasmWindowManager::handle_modify_window(WindowInfo&, WindowSpecification const&)
{

}

void experimental::WasmWindowManager::handle_raise_window(WindowInfo&)
{

}

auto experimental::WasmWindowManager::confirm_placement_on_display(
    WindowInfo const&, MirWindowState, Rectangle const& new_placement) -> Rectangle
{
    return new_placement;
}

bool experimental::WasmWindowManager::handle_keyboard_event(MirKeyboardEvent const*)
{
    return false;
}

bool experimental::WasmWindowManager::handle_touch_event(MirTouchEvent const*)
{
    return false;
}

bool experimental::WasmWindowManager::handle_pointer_event(MirPointerEvent const*)
{
    return false;
}

void experimental::WasmWindowManager::handle_request_move(WindowInfo&, MirInputEvent const*)
{
}

void experimental::WasmWindowManager::handle_request_resize(WindowInfo&, MirInputEvent const*, MirResizeEdge)
{
}

auto experimental::WasmWindowManager::confirm_inherited_move(WindowInfo const&, Displacement) -> Rectangle
{
    return {};
}
