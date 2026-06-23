/*
 * Copyright © Canonical Ltd.
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
 */

fn main() {
    let miral = pkg_config::Config::new()
        .atleast_version("5.8")
        .probe("miral")
        .expect(
            "\n\n\
            ERROR: Could not find the miral C++ library.\n\
            \n\
            The mir-sys crate requires the system-installed miral library (5.8 or later).\n\
            \n\
            On Ubuntu/Debian:\n\
            \n\
            sudo apt install libmiral-dev\n\
            \n\
            For other systems, see: https://github.com/canonical/mir/blob/main/HACKING.md\n\
            \n",
        );

    // Link against miral and its dependencies
    for lib in &miral.libs {
        println!("cargo:rustc-link-lib={}", lib);
    }
    for path in &miral.link_paths {
        println!("cargo:rustc-link-search=native={}", path.display());
    }

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/bridge.cpp");
    println!("cargo:rerun-if-changed=src/bridge.h");
}
