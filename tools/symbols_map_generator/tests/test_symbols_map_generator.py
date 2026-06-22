#!/usr/bin/env python3
# Copyright © Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 or 3,
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""Golden / characterization tests for the symbols-map generator.

These tests parse the small C++ fixture headers in ``fixtures/`` with the real
libclang and assert the exact set of symbol strings the generator emits.  They
serve two purposes:

* As a regression guard: behaviour-preserving refactors of the generator (for
  example reusing a clang Index, skipping function bodies, parallelising the
  parse, or reordering the system-header guard) must not change the emitted
  symbols.  The ``test_*_symbols`` cases below pin the full expected set.

* As living documentation of a bug that still needs fixing: ``has_vtable`` looks
  up base classes on ``node.semantic_parent.get_children()`` instead of
  ``node.get_children()``, so a class that inherits a vtable without declaring
  any virtual method of its own does NOT currently get a ``vtable?for?`` symbol.
  ``test_inherited_vtable_is_detected`` (and the inheritance golden set) assert
  the CORRECT behaviour, so they FAIL on origin/main and pass once the
  has_vtable base-class lookup is fixed.

Run with::

    python3 -m unittest discover -s tools/symbols_map_generator/tests
"""

import importlib.util
import os
import sys
import unittest
from pathlib import Path


TOOL_DIR = Path(__file__).resolve().parent.parent
FIXTURES = Path(__file__).resolve().parent / "fixtures"

# Candidate locations for libclang.so.  The first that exists is used.  Mirrors
# the discovery the generator itself performs in main(), but kept independent so
# the tests can configure libclang before importing the module under test.
_CLANG_SO_CANDIDATES = [
    os.environ.get("MIR_SYMBOLS_MAP_GENERATOR_CLANG_SO_PATH"),
    "/usr/lib/x86_64-linux-gnu/libclang-19.so.1",
    "/usr/lib/x86_64-linux-gnu/libclang-18.so.1",
    "/usr/lib/aarch64-linux-gnu/libclang-19.so.1",
    "/usr/lib/aarch64-linux-gnu/libclang-18.so.1",
    "/usr/lib/llvm-19/lib/libclang.so.1",
    "/usr/lib/llvm-18/lib/libclang.so",
    "/usr/lib/libclang.so.1",
]


def _configure_libclang():
    import clang.cindex

    for candidate in _CLANG_SO_CANDIDATES:
        if candidate and os.path.isfile(candidate):
            clang.cindex.Config.set_library_file(candidate)
            return candidate
    return None


def _load_generator():
    """Import the generator's main.py as a module under a synthetic name."""
    spec = importlib.util.spec_from_file_location(
        "symbols_map_generator_main", TOOL_DIR / "main.py"
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _try_import_clang():
    try:
        import clang.cindex  # noqa: F401

        return True
    except ImportError:
        return False


@unittest.skipUnless(_try_import_clang(), "python3-clang (clang.cindex) is not installed")
class SymbolsMapGeneratorGoldenTest(unittest.TestCase):
    """Parses fixture headers and pins the exact emitted symbol set."""

    @classmethod
    def setUpClass(cls):
        cls._clang_so = _configure_libclang()
        if cls._clang_so is None:
            raise unittest.SkipTest("libclang.so could not be located")
        cls.generator = _load_generator()

    def _symbols_for(self, header_name: str) -> set:
        """Parse a single fixture header and return its emitted symbol set."""
        import clang.cindex

        path = FIXTURES / header_name
        args = ["-std=c++23", "-x", "c++-header"]
        idx = clang.cindex.Index.create()
        tu = idx.parse(path.as_posix(), args=args, options=0)

        # Surface fatal parse diagnostics rather than silently asserting on an
        # empty/garbage symbol set.
        fatal = [
            d
            for d in tu.diagnostics
            if d.severity >= clang.cindex.Diagnostic.Error
        ]
        self.assertEqual(
            [], [d.spelling for d in fatal], f"libclang reported errors parsing {header_name}"
        )

        result: set = set()
        self.generator.traverse_ast(tu.cursor, path.as_posix(), result)
        return result

    def test_plain_symbols(self):
        # A non-polymorphic class: typeinfo but no vtable; private members
        # (hidden(), secret) are never emitted; free functions are emitted.
        self.assertEqual(
            self._symbols_for("plain.h"),
            {
                "mir::Plain::Plain*;",
                "mir::Plain::?Plain*;",
                "mir::Plain::do_thing*;",
                "mir::Plain::value*;",
                "mir::free_function*;",
                "typeinfo?for?mir::Plain;",
            },
        )

    def test_polymorphic_symbols(self):
        # Classes that declare virtual methods (directly or via override) get
        # vtable + typeinfo + non-virtual-thunk symbols.  A pure-virtual method
        # (Abstract::pure) is NOT emitted.
        self.assertEqual(
            self._symbols_for("polymorphic.h"),
            {
                "mir::Base::?Base*;",
                "mir::Base::poly*;",
                "mir::Base::non_virtual*;",
                "mir::Derived::poly*;",
                "mir::Abstract::concrete*;",
                "non-virtual?thunk?to?mir::Base::?Base*;",
                "non-virtual?thunk?to?mir::Base::poly*;",
                "non-virtual?thunk?to?mir::Derived::poly*;",
                "non-virtual?thunk?to?mir::Abstract::concrete*;",
                "typeinfo?for?mir::Base;",
                "typeinfo?for?mir::Derived;",
                "typeinfo?for?mir::Abstract;",
                "vtable?for?mir::Base;",
                "vtable?for?mir::Derived;",
                "vtable?for?mir::Abstract;",
            },
        )

    def test_templates_and_operators_symbols(self):
        # A class template (Holder) emits nothing because it is a CLASS_TEMPLATE
        # cursor, not CLASS_DECL.  Operator overloads normalise to "operator" and
        # therefore collapse to a single deduplicated symbol.
        self.assertEqual(
            self._symbols_for("templates.h"),
            {
                "mir::Number::operator*;",
                "typeinfo?for?mir::Number;",
            },
        )

    def test_inheritance_symbols(self):
        # Golden set for inheritance.h reflecting the CORRECT behaviour: a class
        # that inherits a vtable from a polymorphic base must get its own
        # vtable?for? symbol even when it declares no virtual method of its own.
        # This therefore includes vtable?for?mir::Inheritor / VB1 / VB2 /
        # Diamond.  On origin/main this assertion FAILS because has_vtable looks
        # up base classes on node.semantic_parent.get_children() instead of
        # node.get_children(); it passes once that bug is fixed.
        self.assertEqual(
            self._symbols_for("inheritance.h"),
            {
                "mir::PolyBase::?PolyBase*;",
                "mir::PolyBase::poly*;",
                "mir::Inheritor::non_virtual_helper*;",
                "non-virtual?thunk?to?mir::PolyBase::?PolyBase*;",
                "non-virtual?thunk?to?mir::PolyBase::poly*;",
                "typeinfo?for?mir::PolyBase;",
                "typeinfo?for?mir::Inheritor;",
                "typeinfo?for?mir::VB1;",
                "typeinfo?for?mir::VB2;",
                "typeinfo?for?mir::Diamond;",
                "vtable?for?mir::PolyBase;",
                "vtable?for?mir::Inheritor;",
                "vtable?for?mir::VB1;",
                "vtable?for?mir::VB2;",
                "vtable?for?mir::Diamond;",
                "VTT?for?mir::VB1;",
                "VTT?for?mir::VB2;",
                "VTT?for?mir::Diamond;",
            },
        )

    def test_inherited_vtable_is_detected(self):
        # A class/struct that inherits a vtable from a polymorphic base, but
        # declares no virtual method of its own, must still be emitted with a
        # vtable?for? symbol.  Anything with a typeinfo?for? (and certainly
        # anything with a VTT?for?) is polymorphic and therefore has a vtable;
        # emitting one without the other is internally inconsistent.
        symbols = self._symbols_for("inheritance.h")

        # The base, which declares virtual methods directly, is detected.
        self.assertIn("vtable?for?mir::PolyBase;", symbols)

        # Each inheriting class must also get a vtable, consistent with the
        # typeinfo (and, for virtual bases, VTT) already emitted for it.
        for derived in ("Inheritor", "VB1", "VB2", "Diamond"):
            self.assertIn(f"typeinfo?for?mir::{derived};", symbols)
            self.assertIn(
                f"vtable?for?mir::{derived};",
                symbols,
                f"vtable?for?mir::{derived} is missing — a class that inherits "
                f"a vtable must emit one (has_vtable base-class lookup bug).",
            )

    def test_process_directory_unions_all_headers(self):
        # process_directory should return the union of every header's symbols.
        # This exercises the public entry point (and, on branches that
        # parallelise it, the worker plumbing) and guards against regressions in
        # how per-file results are combined.
        combined = self.generator.process_directory(FIXTURES, [])
        expected = set()
        for header in ("plain.h", "polymorphic.h", "templates.h", "inheritance.h"):
            expected |= self._symbols_for(header)
        self.assertEqual(combined, expected)


class ReadSymbolsFromFileTest(unittest.TestCase):
    """Unit tests for the symbols.map parser, which needs no libclang."""

    @classmethod
    def setUpClass(cls):
        cls.generator = _load_generator()

    def test_parses_versions_and_classifies_c_and_cpp_symbols(self):
        import io

        content = (
            'MIRAL_5.0 {\n'
            'global:\n'
            '  extern "C++" {\n'
            '    mir::Foo::Foo*;\n'
            '    mir::Foo::bar*;\n'
            '  };\n'
            '  some_c_symbol;\n'
            '};\n'
        )
        symbols = self.generator.read_symbols_from_file(io.StringIO(content), "miral")

        names = {s.name for s in symbols}
        self.assertIn("mir::Foo::Foo*;", names)
        self.assertIn("mir::Foo::bar*;", names)
        self.assertIn("some_c_symbol;", names)

        by_name = {s.name: s for s in symbols}
        self.assertFalse(by_name["mir::Foo::Foo*;"].is_c_symbol)
        self.assertTrue(by_name["some_c_symbol;"].is_c_symbol)
        for s in symbols:
            self.assertEqual(s.version, "5.0")


if __name__ == "__main__":
    unittest.main(verbosity=2)
