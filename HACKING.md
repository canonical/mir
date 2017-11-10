Mir hacking guide
=================

Coding *Mir*
----------

There's a *__coding style guide__* in the guides subdirectory. To build it into an
*html* file:

    $ make guides

(Or if you're reading the web version [here](cppguide/index.html)).


Code structure
--------------

*__Code structure: include__*

The include subdirectory contains header files "published" by corresponding parts
of the system. For example, *include/mir/option/option.h* provides a system-wide interface
for accessing runtime options published by the options component.

In many cases, there will be interfaces defined that are used by the component
and implemented elsewhere. *E.g. the compositor uses RenderView which is implemented
by the surfaces component.*

Files under the include directory should contain minimal implementation detail: interfaces
should not expose platform or implementation technology types *etc.* (And as public methods
are normally implementations of interfaces they do not use these types.)


*__Code structure: src__*

This comprises the implementation of *Mir*. Header files for use within the component
should be put here. The only headers from the source tree that should be included are
ones from the current component (ones that do not require a path component).


*__Code structure: test__*

This contains unit, integration and acceptance tests written using *gtest/gmock*. Tests
largely depend upon the public interfaces of components - but tests of units within
a component will include headers from within the source tree.


Error handling strategy
-----------------------

If a function cannot meet its post-conditions it throws an exception and meets
__AT LEAST__ the basic exception safety guarantee. It is a good idea to document the
strong and no-throw guarantees._http://www.boost.org/community/exception_safety.html_

A function is not required to check its preconditions (there should be no
tests that preconditions failures are reported). This means that 
preconditions may be verified using the "*assert"* macro - which may or may
not report problems (depending upon the __NDEBUG__ define).


Implicit rules
--------------

There are a lot of pointers (mostly smart, but a few raw ones) passed
around in the code. We have adopted the general rule that pointers are
expected to refer to valid objects. This avoids repetitive tests for
validity. Unless otherwise documented functions and constructors that
take pointer parameters have validity of the referenced objects as a
precondition. Exceptions to the rule must be of limited scope and 
documented.


Running *Mir*
-----------

There are some brief guides describing how to run the *Mir binaries* once you have
them built. You might think it's obvious but there are some important things
you need to know to get it working, and also to prevent your existing *X server*
from dying at the same time.

 - *\ref getting_and_using_mir*

You can configure *Mir* to provide runtime information helpful for debugging
by enabling component reports:

 - *\ref component_reports*

Documentation
-------------

There are *design notes* and an *architecture diagram* (.dia) in the design
subdirectory.


