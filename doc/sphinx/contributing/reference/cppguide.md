(cppguide)=

# Mir C++ Style Guide

Revision 4.2

Tim Penhey
Neil J. Patel
Thomas Voss

## Background

As every C++ programmer knows, the language has many powerful features,
but this power brings with it complexity, which in turn can make code
more bug-prone and harder to read and maintain.

The goal of this guide is to manage this complexity by describing in
detail the dos and don'ts of writing C++ code. These rules exist to
keep the code base manageable while still allowing coders to use C++
language features productively.

*Style*, also known as readability, is what we call the conventions that
govern our C++ code. The term Style is a bit of a misnomer, since these
conventions cover far more than just source file formatting.

One way in which we keep the code base manageable is by enforcing
*consistency*. It is very important that any programmer be able to look
at another's code and quickly understand it. Maintaining a uniform
style and following conventions means that we can more easily use
"pattern-matching" to infer what various symbols are and what
invariants are true about them. Creating common, required idioms and
patterns makes code much easier to understand. In some cases there might
be good arguments for changing certain style rules, but we nonetheless
keep things as they are in order to preserve consistency.

Another issue this guide addresses is that of C++ feature bloat. C++ is
a huge language with many advanced features. In some cases we constrain,
or even ban, use of certain features. We do this to keep code simple and
to avoid the various common errors and problems that these features can
cause. This guide lists these features and explains why their use is
restricted.

Note that this guide is not a C++ tutorial: we assume that the reader is
familiar with the language.

## Header Files

In general, every `.cpp` file should have an associated `.h` file. There
are some common exceptions, such as unit tests and small `.cpp` files
containing just a `main()` function.

Correct use of header files can make a huge difference to the
readability, size and performance of your code.

The following rules will guide you through the various pitfalls of using
header files.

### The #define Guard

All header files should have `#define` guards to prevent multiple
inclusion. The format of the symbol name should be
*`<PROJECT>`*`_`*`<PATH>`*`_`*`<FILE>`*`_H_`.

To guarantee uniqueness, they should be based on the full path in a
project's source tree. For example, the file `foo/src/bar/baz.h` in
project `foo` should have the following guard:

```c++
#ifndef FOO_BAR_BAZ_H_
#define FOO_BAR_BAZ_H_

...

#endif  // FOO_BAR_BAZ_H_
```

### Header File Dependencies

Don't use an `#include` when a forward declaration would suffice.

When you include a header file you introduce a dependency that will
cause your code to be recompiled whenever the header file changes. If
your header file includes other header files, any change to those files
will cause any code that includes your header to be recompiled.
Therefore, we prefer to minimize includes, particularly includes of
header files in other header files.

You can significantly reduce the number of header files you need to
include in your own header files by using forward declarations. For
example, if your header file uses the `File` class in ways that do not
require access to the declaration of the `File` class, your header file
can just forward declare `class File;` instead of having to
`#include "file/base/file.h"`.

How can we use a class `Foo` in a header file without access to its
definition?

- We can declare data members of type `Foo*` or `Foo&`.
- We can declare (but not define) functions with arguments, and/or
  return values, of type `Foo`. (One exception is if an argument `Foo`
  or `Foo const&` has a non-`explicit`, one-argument constructor, in
  which case we need the full definition to support automatic type
  conversion.)
- We can declare static data members of type `Foo`. This is because
  static data members are defined outside the class definition.

On the other hand, you must include the header file for `Foo` if your
class subclasses `Foo` or has a data member of type `Foo`.

Sometimes it makes sense to have pointer (or better, `unique_ptr`)
members instead of object members. However, this complicates code
readability and imposes a performance penalty, so avoid doing this
transformation if the only purpose is to minimize includes in header
files.

Of course, `.cpp` files typically do require the definitions of the
classes they use, and usually have to include several header files.

**Note:** If you use a symbol `Foo` in your source file, you should
bring in a definition for `Foo` yourself, either via an #include or via
a forward declaration. Do not depend on the symbol being brought in
transitively via headers not directly included. One exception is if
`Foo` is used in `myfile.cpp`, it's ok to #include (or
forward-declare) `Foo` in `myfile.h`, instead of `myfile.cpp`.

### Inline Functions

Define functions inline only when they are small, say, 10 lines or less.

**Definition:**

You can declare functions in a way that allows the compiler to expand
them inline rather than calling them through the usual function call
mechanism.

**Pros:**

Inlining a function can generate more efficient object code, as long as
the inlined function is small. Feel free to inline accessors and
mutators, and other short, performance-critical functions.

**Cons:**

Overuse of inlining can actually make programs slower. Depending on a
function's size, inlining it can cause the code size to increase or
decrease. Inlining a very small accessor function will usually decrease
code size while inlining a very large function can dramatically increase
code size. On modern processors smaller code usually runs faster due to
better use of the instruction cache.

**Decision:**

A decent rule of thumb is to not inline a function if it is more than 10
lines long. Beware of destructors, which are often longer than they
appear because of implicit member- and base-destructor calls!

Another useful rule of thumb: it's typically not cost effective to
inline functions with loops or switch statements (unless, in the common
case, the loop or switch statement is never executed).

It is important to know that functions are not always inlined even if
they are declared as such; for example, virtual and recursive functions
are not normally inlined. Usually recursive functions should not be
inline. The main reason for making a virtual function inline is to place
its definition in the class, either for convenience or to document its
behavior, e.g., for accessors and mutators.

(the-inl-h-files)=

### The -inl.h Files

You may use file names with a `-inl.h` suffix to define complex inline
functions when needed.

The definition of an inline function needs to be in a header file, so
that the compiler has the definition available for inlining at the call
sites. However, implementation code properly belongs in `.cpp` files,
and we do not like to have much actual code in `.h` files unless there
is a readability or performance advantage.

If an inline function definition is short, with very little, if any,
logic in it, you should put the code in your `.h` file. For example,
accessors and mutators should certainly be inside a class definition.
More complex inline functions may also be put in a `.h` file for the
convenience of the implementer and callers, though if this makes the
`.h` file too unwieldy you can instead put that code in a separate
`-inl.h` file. This separates the implementation from the class
definition, while still allowing the implementation to be included where
necessary.

Another use of `-inl.h` files is for definitions of function templates.
This can be used to keep your template definitions easy to read.

Do not forget that a `-inl.h` file requires a [`#define`
guard](#the-define-guard) just like any other header file.

### Function Parameter Ordering

When defining a function, parameter order is: outputs, then inputs.

Parameters to C/C++ functions are either input to the function, output
from the function, or both. Input parameters are usually values or
`const` references, while output and input/output parameters will be
non-`const` references or pointers to non-`const`. When ordering
function parameters, put all output parameters before any input-only
parameters. In particular, do not add new parameters to the end of the
function just because they are new; place new output parameters before
the input-only parameters.

This is not a hard-and-fast rule. Parameters that are both input and
output (often classes/structs) muddy the waters, and, as always,
consistency with related functions may require you to bend the rule.

### Names and Order of Includes

Use standard order for readability and to avoid hidden dependencies:
your project's public `.h`, your project's private `.h`, other
libraries' `.h`, .C library, C++ library,

All of a project's header files should be listed as descendants of the
project's source directory without use of UNIX directory shortcuts `.`
(the current directory) or `..` (the parent directory). For example,
`my-awesome-project/src/base/logging.h` should be included as

```c++
#include "base/logging.h"
```

In `dir/foo.cpp` or `dir/foo_test.cpp`, whose main purpose is to
implement or test the stuff in `dir2/foo2.h`, order your includes as
follows:

1. `dir2/foo2.h` (preferred location --- see details below).
1. Your project's public `.h` files.
1. Your project's private `.h` files.
1. Other libraries' `.h` files.
1. C system files.
1. C++ system files.

The preferred ordering reduces hidden dependencies. We want every header
file to be compilable on its own. The easiest way to achieve this is to
make sure that every one of them is the first `.h` file `#include`d in
some `.cpp`.

`dir/foo.cpp` and `dir2/foo2.h` are often in the same directory (e.g.
`base/test_basictypes.cpp` and `base/basictypes.h`), but can be in
different directories too.

Within each section it is nice to order the includes alphabetically.

For example, the includes in
`my-awesome-project/src/foo/internal/fooserver.cpp` might look like
this:

```c++
#include "foo/public/fooserver.h"  // Preferred location.

#include "base/basictypes.h"
#include "base/commandlineflags.h"
#include "foo/public/bar.h"

#include <sys/types.h>
#include <unistd.h>

#include <hash_map>
#include <vector>
```

## Scoping

### Namespaces

Unnamed namespaces in `.cpp` files are encouraged. With named
namespaces, choose the name based on the project, and possibly its path.
Do not use a *using-directive* in a header file.

**Definition:**

Namespaces subdivide the global scope into distinct, named scopes, and
so are useful for preventing name collisions in the global scope.

**Pros:**

Namespaces provide a (hierarchical) axis of naming, in addition to the
(also hierarchical) name axis provided by classes.

For example, if two different projects have a class `Foo` in the global
scope, these symbols may collide at compile time or at runtime. If each
project places their code in a namespace, `project1::Foo` and
`project2::Foo` are now distinct symbols that do not collide.

**Cons:**

Namespaces can be confusing, because they provide an additional
(hierarchical) axis of naming, in addition to the (also hierarchical)
name axis provided by classes.

Use of unnamed spaces in header files can easily cause violations of the
C++ One Definition Rule (ODR).

**Decision:**

Use namespaces according to the policy described below.

**Unnamed Namespaces**

- Unnamed namespaces are allowed and even encouraged in `.cpp` files, to
  avoid runtime naming conflicts:

  ```c++
  namespace                           // This is in a .cpp file.
  {
  // The content of a namespace is not indented
  enum { UNUSED, EOF, ERROR };       // Commonly used tokens.
  bool AtEof() { return pos_ == EOF; }  // Uses our namespace's EOF.

  }  // namespace
  ```

  However, file-scope declarations that are associated with a particular
  class may be declared in that class as types, static data members or
  static member functions rather than as members of an unnamed
  namespace. Terminate the unnamed namespace as shown, with a comment
  `// namespace`.

- Do not use unnamed namespaces in `.h` files.

**Named Namespaces**

Named namespaces should be used as follows:

- Namespaces wrap the entire source file after includes,
  [gflags](https://github.com/gflags/gflags) definitions/declarations,
  and forward declarations of classes from other namespaces:

  ```c++
  // In the .h file
  namespace mynamespace
  {

  // All declarations are within the namespace scope.
  // Notice the lack of indentation.
  class MyClass
  {
  public:
      ...
      void foo();
  };

  }  // namespace mynamespace
  ```

  ```c++
  // In the .cpp file
  namespace mynamespace
  {

  // Definition of functions is within scope of the namespace.
  void MyClass::foo()
  {
      ...
  }

  }  // namespace mynamespace
  ```

  The typical `.cpp` file might have more complex detail, including the
  need to reference classes in other namespaces.

  ```c++
  #include "a.h"

  DEFINE_BOOL(someflag, false, "placeholder flag");

  class C;  // Forward declaration of class C in the global namespace.
  namespace a { class A; }  // Forward declaration of a::A.

  namespace b
  {

  ...code for b...         // Code goes against the left margin.

  }  // namespace b
  ```

- Do not declare anything in namespace `std`, not even forward
  declarations of standard library classes. Declaring entities in
  namespace `std` is undefined behavior, i.e., not portable. To declare
  entities from the standard library, include the appropriate header
  file.

- You may use a *using-directive* to make all names from a namespace
  available, but only in a source file.

- You may use a *using-declaration* anywhere in a `.cpp` file, and in
  functions, methods or classes in `.h` files.

  ```c++
  // OK in .cpp files.
  // Must be in a function, method or class in .h files.
  using ::foo::bar;
  ```

- Namespace aliases are allowed anywhere in a `.cpp` file, anywhere
  inside the named namespace that wraps an entire `.h` file, and in
  functions and methods.

  ```c++
  // Shorten access to some commonly used names in .cpp files.
  namespace fbz = ::foo::bar::baz;

  // Shorten access to some commonly used names (in a .h file).
  namespace librarian
  {
  // The following alias is available to all files including
  // this header (in namespace librarian):
  // alias names should therefore be chosen consistently
  // within a project.
  namespace pd_s = ::pipeline_diagnostics::sidetable;

  inline void my_inline_function()
  {
      // namespace alias local to a function (or method).
      namespace fbz = ::foo::bar::baz;
      ...
  }
  }  // namespace librarian
  ```

  Note that an alias in a .h file is visible to everyone #including
  that file, so public headers (those available outside a project) and
  headers transitively #included by them, should avoid defining
  aliases, as part of the general goal of keeping public APIs as small
  as possible.

### Nested Classes

Although you may use public nested classes when they are part of an
interface, consider a [namespace](#namespaces) to keep declarations out
of the global scope.

**Definition:**

A class can define another class within it; this is also called a
*member class*.

```c++
class Foo
{
private:
    // Bar is a member class, nested within Foo.
    class Bar
    {
      ...
    };

};
```

**Pros:**

This is useful when the nested (or member) class is only used by the
enclosing class; making it a member puts it in the enclosing class scope
rather than polluting the outer scope with the class name. Nested
classes can be forward declared within the enclosing class and then
defined in the `.cpp` file to avoid including the nested class
definition in the enclosing class declaration, since the nested class
definition is usually only relevant to the implementation.

**Cons:**

Nested classes can be forward-declared only within the definition of the
enclosing class. Thus, any header file manipulating a `Foo::Bar*`
pointer will have to include the full class declaration for `Foo`.

**Decision:**

Do not make nested classes public unless they are actually part of the
interface, e.g., a class that holds a set of options for some method.

### Nonmember, Static Member, and Global Functions

Prefer nonmember functions within a namespace or static member functions
to global functions; use completely global functions rarely.

**Pros:**

Nonmember and static member functions can be useful in some situations.
Putting nonmember functions in a namespace avoids polluting the global
namespace.

**Cons:**

Nonmember and static member functions may make more sense as members of
a new class, especially if they access external resources or have
significant dependencies.

**Decision:**

Sometimes it is useful, or even necessary, to define a function not
bound to a class instance. Such a function can be either a static member
or a nonmember function. Nonmember functions should not depend on
external variables, and should nearly always exist in a namespace.
Rather than creating classes only to group static member functions which
do not share static data, use [namespaces](#namespaces) instead.

Functions defined in the same compilation unit as production classes may
introduce unnecessary coupling and link-time dependencies when directly
called from other compilation units; static member functions are
particularly susceptible to this. Consider extracting a new class, or
placing the functions in a namespace possibly in a separate library.

If you must define a nonmember function and it is only needed in its
`.cpp` file, use an unnamed [namespace](#namespaces) or `static` linkage
(eg `static int foo() {...}`) to limit its scope.

### Local Variables

Place a function's variables in the narrowest scope possible, and
initialize variables in the declaration.

C++ allows you to declare variables anywhere in a function. We encourage
you to declare them in as local a scope as possible, and as close to the
first use as possible. This makes it easier for the reader to find the
declaration and see what type the variable is and what it was
initialized to. In particular, initialization should be used instead of
declaration and assignment, e.g.

<!-- badcode -->

```c++
int i;
i = f();      // Bad -- initialization separate from declaration.
```

```c++
int j = g();  // Good -- declaration has initialization.
```

Note that gcc implements `for (int i = 0; i < 10; ++i)` correctly (the
scope of `i` is only the scope of the `for` loop), so you can then reuse
`i` in another `for` loop in the same scope. It also correctly scopes
declarations in `if` and `while` statements, e.g.

```c++
while (char const* p = strchr(str, '/')) str = p + 1;
```

There is one caveat: if the variable is an object, its constructor is
invoked every time it enters scope and is created, and its destructor is
invoked every time it goes out of scope.

<!-- badcode -->

```c++
// Inefficient implementation:
for (int i = 0; i < 1000000; ++i)
{
    Foo f;  // My ctor and dtor get called 1000000 times each.
    f.do_something(i);
}
```

It may be more efficient to declare such a variable used in a loop
outside that loop:

```c++
Foo f;  // My ctor and dtor get called once each.
for (int i = 0; i < 1000000; ++i)
{
    f.do_something(i);
}
```

## Classes

Classes are the fundamental unit of code in C++. Naturally, we use them
extensively. This section lists the main dos and don'ts you should
follow when writing a class.

### Constructors

The purpose of a constructor is to initialise a class so that its
invariants hold. For value classes it is worth having a cheap default
constructor.

**Definition:**

It is possible to perform initialization in the body of the constructor.

**Pros:**

Convenience in typing. No need to worry about whether the class has been
initialized or not.

**Cons:**

The problems with doing work in constructors are:

- If the work calls virtual functions, these calls will not get
  dispatched to the subclass implementations. Future modification to
  your class can quietly introduce this problem even if your class is
  not currently subclassed, causing much confusion.
- If someone creates a global variable of this type (which is against
  the rules, but still), the constructor code will be called before
  `main()`, possibly breaking some implicit assumptions in the
  constructor code.

**Decision:**

Constructors should not make virtual calls to functions, access
potentially uninitialized global variables, etc.

### Default Constructors

You must define a default constructor if your class defines member
variables of POD types and has no other constructors. Otherwise the
compiler will do it for you, badly.

**Definition:**

The default constructor is called when we create a class object with no
arguments. It is always called when calling `new[]` (for arrays).

**Pros:**

Initializing structures by default makes debugging much easier.

**Cons:**

Extra work for you, the code writer.

**Decision:**

If your class defines POD member variables and has no other constructors
you must define a default constructor (one that takes no arguments). It
should initialize the object in such a way that its internal state is
consistent and valid.

The reason for this is that if you have no other constructors and do not
define a default constructor, the compiler will generate one for you.
This compiler generated constructor may not initialize your object
sensibly.

If your class is composed from and/or inherits from an existing class or
classes but you add no new member variables, you are not required to
have a default constructor.

If your class has value semantics then consider making the class
invariants such that the default constructor is cheap. For example,
initialising member pointers to `nullptr` and allocating on first use.

### Explicit Constructors

Use the C++ keyword `explicit` for constructors with one argument.

**Definition:**

Normally, if a constructor takes one argument, it can be used as a
conversion. For instance, if you define `Foo::Foo(string name)` and then
pass a string to a function that expects a `Foo`, the constructor will
be called to convert the string into a `Foo` and will pass the `Foo` to
your function for you. This can be convenient but is also a source of
trouble when things get converted and new objects created without you
meaning them to. Declaring a constructor `explicit` prevents it from
being invoked implicitly as a conversion.

**Pros:**

Avoids undesirable conversions.

**Cons:**

Avoids desirable conversions.

**Decision:**

We require all single argument constructors to be explicit. Always put
`explicit` in front of one-argument constructors in the class
definition: `explicit Foo(string name);`

The exception is copy constructors, which, in the rare cases when we
allow them, should probably not be `explicit`. Classes that are intended
to be transparent wrappers around other classes are also exceptions.
Such exceptions should be clearly marked with comments.

### Copy Constructors

Provide a copy constructor and assignment operator only when necessary.
Otherwise, disable them with the help of `= delete;`.

**Definition:**

The copy constructor and assignment operator are used to create copies
of objects. The copy constructor is implicitly invoked by the compiler
in some situations, e.g. passing objects by value.

**Pros:**

Copy constructors make it easy to copy objects. STL containers require
that all contents be copyable and assignable. Copy constructors can be
more efficient than `CopyFrom()`-style workarounds because they combine
construction with copying, the compiler can elide them in some contexts,
and they make it easier to avoid heap allocation.

**Cons:**

Implicit copying of objects in C++ is a rich source of bugs and of
performance problems. It also reduces readability, as it becomes hard to
track which objects are being passed around by value as opposed to by
reference, and therefore where changes to an object are reflected.

**Decision:**

Few classes need to be copyable. Most should have neither a copy
constructor nor an assignment operator. In many situations, a pointer or
reference will work just as well as a copied value, with better
performance. For example, you can pass function parameters by reference
or pointer instead of by value, and you can store pointers rather than
objects in an STL container.

If your class needs to be copyable, prefer providing a copy method, such
as `CopyFrom()` or `Clone()`, rather than a copy constructor, because
such methods cannot be invoked implicitly. If a copy method is
insufficient in your situation (e.g. for performance reasons, or because
your class needs to be stored by value in an STL container), provide
both a copy constructor and assignment operator.

If your class does not need a copy constructor or assignment operator,
you must explicitly disable them.

### Structs vs. Classes

Use a `struct` only for passive objects that carry data; everything else
is a `class`.

The `struct` and `class` keywords behave almost identically in C++. We
add our own semantic meanings to each keyword, so you should use the
appropriate keyword for the data-type you're defining.

`structs` should be used for passive objects that carry data, and may
have associated constants, but lack any functionality other than
access/setting the data members. The accessing/setting of fields is done
by directly accessing the fields rather than through method invocations.
Methods should not provide behavior but should only be used to set up
the data members, e.g., constructor, destructor, `initialize()`,
`reset()`, `validate()`.

If more functionality is required, a `class` is more appropriate. If in
doubt, make it a `class`.

For consistency with STL, you can use `struct` instead of `class` for
functors and traits.

### Inheritance

Composition is often more appropriate than inheritance. When using
inheritance, make it `public`.

**Definition:**

When a sub-class inherits from a base class, it includes the definitions
of all the data and operations that the parent base class defines. In
practice, inheritance is used in two major ways in C++: implementation
inheritance, in which actual code is inherited by the child, and
[interface inheritance](#interfaces), in which only method names are
inherited.

**Pros:**

Implementation inheritance reduces code size by re-using the base class
code as it specializes an existing type. Because inheritance is a
compile-time declaration, you and the compiler can understand the
operation and detect errors. Interface inheritance can be used to
programmatically enforce that a class expose a particular API. Again,
the compiler can detect errors, in this case, when a class does not
define a necessary method of the API.

**Cons:**

For implementation inheritance, because the code implementing a
sub-class is spread between the base and the sub-class, it can be more
difficult to understand an implementation. The sub-class cannot override
functions that are not virtual, so the sub-class cannot change
implementation. The base class may also define some data members, so
that specifies physical layout of the base class.

**Decision:**

All inheritance should be `public`. If you want to do private
inheritance, you should be including an instance of the base class as a
member instead.

Do not overuse implementation inheritance. Composition is often more
appropriate. Try to restrict use of inheritance to the "is-a" case:
`Bar` subclasses `Foo` if it can reasonably be said that `Bar` "is a
kind of" `Foo`.

Make your destructor `virtual` if necessary. If your class has virtual
methods, its destructor should be virtual.

Limit the use of `protected` to those member functions that might need
to be accessed from subclasses. Note that [data members should be
private](#access-control).

When redefining an inherited virtual method (both pure and non-pure),
explicitly declare it `override` in the declaration of the derived
class. Rationale: using `override` allows the compiler to consistently
detect attempts to override methods that have been changed or completely
removed. It also makes it straightforward for a reader to determine if a
method is virtual or not.

### Multiple Inheritance

Only very rarely is multiple inheritance of implementation actually
useful. We allow multiple inheritance only when at most one of the base
classes has an implementation; all other base classes must be
[interface](#interfaces) classes.

**Definition:**

Multiple inheritance allows a sub-class to have more than one base
class. We distinguish between base classes that are *interfaces* and
those that have an *implementation*.

**Pros:**

Multiple implementation inheritance may let you re-use even more code
than single inheritance (see [Inheritance](#inheritance)).

**Cons:**

Only very rarely is multiple *implementation* inheritance actually
useful. When multiple implementation inheritance seems like the
solution, you can usually find a different, more explicit, and cleaner
solution.

**Decision:**

Multiple inheritance is allowed only when all superclasses, with the
possible exception of the first one, are [interfaces](#interfaces).

### Interfaces

Classes that satisfy certain conditions are interfaces.

**Definition:**

A class is an interface if it meets the following requirements:

- It has only public pure virtual ("`= 0`") methods and static methods
  (but see below for destructor).
- It may not have non-static data members.
- It need not have any constructors defined. If a constructor is
  provided, it must take no arguments and it must be protected.
- If it is a subclass, it may only be derived from classes that satisfy
  these conditions.

An interface class can never be directly instantiated because of the
pure virtual method(s) it declares. To make sure all implementations of
the interface can be destroyed correctly, they must also declare a
virtual, or protected, destructor (in an exception to the first rule,
this should not be pure). See Stroustrup, The C++ Programming Language,
3rd edition, section 12.4 for details.

### Operator Overloading

Overload operators where appropriate.

**Definition:**

A class can define that operators such as `+` and `/` operate on the
class as if it were a built-in type.

**Pros:**

Can make code appear more intuitive because a class will behave in the
same way as built-in types (such as `int`). Overloaded operators are
more playful names for functions that are less-colorfully named, such as
`equals()` or `add()`. For some template functions to work correctly,
you may need to define operators.

**Cons:**

While operator overloading can make code more intuitive, it has several
drawbacks:

- It can fool our intuition into thinking that expensive operations are
  cheap, built-in operations.
- It is much harder to find the call sites for overloaded operators.
  Searching for `Equals()` is much easier than searching for relevant
  invocations of `==`.
- Some operators work on pointers too, making it easy to introduce bugs.
  `Foo + 4` may do one thing, while `&Foo + 4` does something totally
  different. The compiler does not complain for either of these, making
  this very hard to debug.

Overloading also has surprising ramifications. For instance, if a class
overloads unary `operator&`, it cannot safely be forward-declared.

**Decision:**

In general, do overload operators where appropriate.

See also [Copy Constructors](#copy-constructors) and [Function
Overloading](#function-overloading).

### Access Control

Make data members `private`, and provide access to them through accessor
functions as needed (for technical reasons, we allow data members of a
test fixture class to be `protected` when using [Google
Test](https://github.com/google/googletest)). Typically a variable would
be called `foo` and the accessor function `get_foo()`. You may also want
a mutator function `set_foo()`. Exception: `static const` data members
need not be `private`.

The definitions of accessors are usually inlined in the header file.

See also [Inheritance](#inheritance) and [Function
Names](#function-names).

### Declaration Order

Use the specified order of declarations within a class: `public:` before
`private:`, methods before data members (variables), etc.

Your class definition should start with its `public:` section, followed
by its `protected:` section and then its `private:` section. If any of
these sections are empty, omit them.

Within each section, the declarations generally should be in the
following order:

- Typedefs and Enums
- Constants (`static const` data members)
- Constructors
- Destructor
- Methods, including static methods
- Data Members (except `static const` data members)

Friend declarations should always be in the private section, and the
`DISALLOW_COPY_AND_ASSIGN` macro invocation should be at the end of the
`private:` section. It should be the last thing in the class. See [Copy
Constructors](#copy-constructors).

Method definitions in the corresponding `.cpp` file should be the same
as the declaration order, as much as possible.

Do not put large method definitions inline in the class definition.
Usually, only trivial or performance-critical, and very short, methods
may be defined inline. See [Inline Functions](#inline-functions) for
more details.

### Write Short Functions

Prefer small and focused functions.

We recognize that long functions are sometimes appropriate, so no hard
limit is placed on functions length. If a function exceeds about 40
lines, think about whether it can be broken up without harming the
structure of the program.

Even if your long function works perfectly now, someone modifying it in
a few months may add new behavior. This could result in bugs that are
hard to find. Keeping your functions short and simple makes it easier
for other people to read and modify your code.

You could find long and complicated functions when working with some
code. Do not be intimidated by modifying existing code: if working with
such a function proves to be difficult, you find that errors are hard to
debug, or you want to use a piece of it in several different contexts,
consider breaking up the function into smaller and more manageable
pieces.

## Other C++ Features

### Reference Arguments

Most parameters passed by reference should be labeled `const`.

**Definition:**

In C, if a function needs to modify a variable, the parameter must use a
pointer, eg `int foo(int *pval)`. In C++, the function can alternatively
declare a reference parameter: `int foo(int& val)`.

**Pros:**

Defining a parameter as reference avoids ugly code like `(*pval)++`.
Necessary for some applications like copy constructors. Makes it clear,
unlike with pointers, that `NULL` is not a possible value.

**Cons:**

References can be confusing, as they have value syntax but pointer
semantics.

**Decision:**

Within function parameter lists all references must be `const`:

```c++
void foo(string const& in, string* out);
```

In fact it is a very strong convention in Unity code that input
arguments are values or `const` references while output arguments are
pointers. Input parameters may be `const` pointers. Non-`const`
reference parameters are allowed but there must be a valid reason for
it, a strong preference is given to `const` reference parameters.

One case when you might want an input parameter to be a `const` pointer
is if you want to emphasize that the argument is not copied, so it must
exist for the lifetime of the object; it is usually best to document
this in comments as well. STL adapters such as `bind2nd` and `mem_fun`
do not permit reference parameters, so you must declare functions with
pointer parameters in these cases, too.

### Function Overloading

Use overloaded functions (including constructors) only if a reader
looking at a call site can get a good idea of what is happening without
having to first figure out exactly which overload is being called.

**Definition:**

You may write a function that takes a `string const&` and overload it
with another that takes `char const*`.

```c++
class MyClass {
public:
    void analyze(string const& text);
    void analyze(char const* text, size_t textlen);
};
```

**Pros:**

Overloading can make code more intuitive by allowing an
identically-named function to take different arguments. It may be
necessary for templatized code, and it can be convenient for Visitors.

**Cons:**

If a function is overloaded by the argument types alone, a reader may
have to understand C++'s complex matching rules in order to tell
what's going on. Also many people are confused by the semantics of
inheritance if a derived class overrides only some of the variants of a
function.

**Decision:**

If you want to overload a function, consider qualifying the name with
some information about the arguments, e.g., `append_string()`,
`AppendInt()` rather than just `append()`.

### Default Arguments

We do not allow default function parameters, except in a few uncommon
situations explained below.

**Pros:**

Often you have a function that uses lots of default values, but
occasionally you want to override the defaults. Default parameters allow
an easy way to do this without having to define many functions for the
rare exceptions.

**Cons:**

People often figure out how to use an API by looking at existing code
that uses it. Default parameters are more difficult to maintain because
copy-and-paste from previous code may not reveal all the parameters.
Copy-and-pasting of code segments can cause major problems when the
default arguments are not appropriate for the new code.

**Decision:**

Except as described below, we require all arguments to be explicitly
specified, to force programmers to consider the API and the values they
are passing for each argument rather than silently accepting defaults
they may not be aware of.

One specific exception is when default arguments are used to simulate
variable-length argument lists.

```c++
// Support up to 4 params by using a default empty AlphaNum.
string str_cat(AlphaNum const& a,
               AlphaNum const& b = gEmptyAlphaNum,
               AlphaNum const& c = gEmptyAlphaNum,
               AlphaNum const& d = gEmptyAlphaNum);
```

### Variable-Length Arrays and alloca()

We do not allow variable-length arrays or `alloca()`.

**Pros:**

Variable-length arrays have natural-looking syntax. Both variable-length
arrays and `alloca()` are very efficient.

**Cons:**

Variable-length arrays and alloca are not part of Standard C++. More
importantly, they allocate a data-dependent amount of stack space that
can trigger difficult-to-find memory overwriting bugs: "It ran fine on
my machine, but dies mysteriously in production".

**Decision:**

Use a safe allocator instead, such as `unique_ptr`.

### Friends

We allow use of `friend` classes and functions, within reason.

Friends should usually be defined in the same file so that the reader
does not have to look in another file to find uses of the private
members of a class. A common use of `friend` is to have a `FooBuilder`
class be a friend of `Foo` so that it can construct the inner state of
`Foo` correctly, without exposing this state to the world. In some cases
it may be useful to make a unit test class a friend of the class it
tests.

Friends extend, but do not break, the encapsulation boundary of a class.
In some cases this is better than making a member public when you want
to give only one other class access to it. However, most classes should
interact with other classes solely through their public members.

### Casting

Use C++ casts like `static_cast<>()`. Do not use other cast formats like
`int y = (int)x;` or `int y = int(x);`.

**Definition:**

C++ introduced a different cast system from C that distinguishes the
types of cast operations.

**Pros:**

The problem with C casts is the ambiguity of the operation; sometimes
you are doing a *conversion* (e.g., `(int)3.5`) and sometimes you are
doing a *cast* (e.g., `(int)"hello"`); C++ casts avoid this.
Additionally C++ casts are more visible when searching for them.

**Cons:**

The syntax is nasty.

**Decision:**

Do not use C-style casts. Instead, use these C++-style casts.

- Use `static_cast` as the equivalent of a C-style cast that does value
  conversion, or when you need to explicitly up-cast a pointer from a
  class to its superclass.
- Use `const_cast` to remove the `const` qualifier (see
  [const](#use-of-const)).
- Use `reinterpret_cast` to do unsafe conversions of pointer types to
  and from integer and other pointer types. Use this only if you know
  what you are doing and you understand the aliasing issues.
- Do not use `dynamic_cast` except in test code. If you need to know
  type information at runtime in this way outside of a unit test, you
  probably have a design flaw.

### Streams

Use streams only for logging.

**Definition:**

Streams are a replacement for `printf()` and `scanf()`.

**Pros:**

With streams, you do not need to know the type of the object you are
printing. You do not have problems with format strings not matching the
argument list. (Though with gcc, you do not have that problem with
`printf` either.) Streams have automatic constructors and destructors
that open and close the relevant files.

**Cons:**

Streams make it difficult to do functionality like `pread()`. Some
formatting (particularly the common format string idiom `%.*s`) is
difficult if not impossible to do efficiently using streams without
using `printf`-like hacks. Streams do not support operator reordering
(the `%1s` directive), which is helpful for internationalization.

**Decision:**

Do not use streams, except where required by a logging interface. Use
`printf`-like routines instead.

There are various pros and cons to using streams, but in this case, as
in many other cases, consistency trumps the debate. Do not use streams
in your code.

**Extended Discussion**

There has been debate on this issue, so this explains the reasoning in
greater depth. Recall the Only One Way guiding principle: we want to
make sure that whenever we do a certain type of I/O, the code looks the
same in all those places. Because of this, we do not want to allow users
to decide between using streams or using `printf` plus Read/Write/etc.
Instead, we should settle on one or the other. We made an exception for
logging because it is a pretty specialized application, and for
historical reasons.

Proponents of streams have argued that streams are the obvious choice of
the two, but the issue is not actually so clear. For every advantage of
streams they point out, there is an equivalent disadvantage. The biggest
advantage is that you do not need to know the type of the object to be
printing. This is a fair point. But, there is a downside: you can easily
use the wrong type, and the compiler will not warn you. It is easy to
make this kind of mistake without knowing when using streams.

```c++
cout << this;  // Prints the address
cout << *this;  // Prints the contents
```

The compiler does not generate an error because `<<` has been
overloaded. We discourage overloading for just this reason.

Some say `printf` formatting is ugly and hard to read, but streams are
often no better. Consider the following two fragments, both with the
same typo. Which is easier to discover?

```c++
cerr << "Error connecting to '" << foo->bar()->hostname.first
     << ":" << foo->bar()->hostname.second << ": " << strerror(errno);

fprintf(stderr, "Error connecting to '%s:%u: %s",
        foo->bar()->hostname.first, foo->bar()->hostname.second,
        strerror(errno));
```

And so on and so forth for any issue you might bring up. (You could
argue, "Things would be better with the right wrappers," but if it is
true for one scheme, is it not also true for the other? Also, remember
the goal is to make the language smaller, not add yet more machinery
that someone has to learn.)

Either path would yield different advantages and disadvantages, and
there is not a clearly superior solution. The simplicity doctrine
mandates we settle on one of them though, and the majority decision was
on `printf` + `read`/`write`.

### Preincrement and Predecrement

Use prefix form (`++i`) of the increment and decrement operators with
iterators and other template objects.

**Definition:**

When a variable is incremented (`++i` or `i++`) or decremented (`--i` or
`i--`) and the value of the expression is not used, one must decide
whether to preincrement (decrement) or postincrement (decrement).

**Pros:**

When the return value is ignored, the "pre" form (`++i`) is never less
efficient than the "post" form (`i++`), and is often more efficient.
This is because post-increment (or decrement) requires a copy of `i` to
be made, which is the value of the expression. If `i` is an iterator or
other non-scalar type, copying `i` could be expensive. Since the two
types of increment behave the same when the value is ignored, why not
just always pre-increment?

**Cons:**

The tradition developed, in C, of using post-increment when the
expression value is not used, especially in `for` loops. Some find
post-increment easier to read, since the "subject" (`i`) precedes the
"verb" (`++`), just like in English.

**Decision:**

For simple scalar (non-object) values there is no reason to prefer one
form and we allow either. For iterators and other template types, use
pre-increment.

### Use of const

We strongly recommend that you use `const` whenever it makes sense to do
so.

**Definition:**

Declared variables and parameters can be preceded by the keyword `const`
to indicate the variables are not changed (e.g., `int const foo`). Class
functions can have the `const` qualifier to indicate the function does
not change the state of the class member variables (e.g.,
`class Foo { int bar(char c) const; };`).

**Pros:**

Easier for people to understand how variables are being used. Allows the
compiler to do better type checking, and, conceivably, generate better
code. Helps people convince themselves of program correctness because
they know the functions they call are limited in how they can modify
your variables. Helps people know what functions are safe to use without
locks in multi-threaded programs.

**Cons:**

`const` is viral: if you pass a `const` variable to a function, that
function must have `const` in its prototype (or the variable will need a
`const_cast`). This can be a particular problem when calling library
functions.

**Decision:**

`const` variables, data members, methods and arguments add a level of
compile-time type checking; it is better to detect errors as soon as
possible. Therefore we strongly recommend that you use `const` whenever
it makes sense to do so:

- If a function does not modify an argument passed by reference or by
  pointer, that argument should be `const`.
- Declare methods to be `const` whenever possible. Accessors should
  almost always be `const`. Other methods should be const if they do not
  modify any data members, do not call any non-`const` methods, and do
  not return a non-`const` pointer or non-`const` reference to a data
  member.
- Consider making data members `const` whenever they do not need to be
  modified after construction.

However, do not go crazy with `const`. Something like
`int const* const* const x;` is likely overkill, even if it accurately
describes how const x is. Focus on what's really useful to know: in
this case, `int const** x` is probably sufficient.

The `mutable` keyword is allowed but is unsafe when used with threads,
so thread safety should be carefully considered first.

**Where to put the const**

We favor the form `int const* foo` to `const int* foo`. This keeps the
`const` with the type modifier (& or \*).

That said, while we encourage putting `const` after the type, we do not
require it. But be consistent with the code around you!

### Integer Types

Use built-in C++ integer types, both signed and unsigned int. Use more
specific types like `size_t` where appropriate. If a program needs a
variable of a different size, use a precise-width integer type from
`<cstdint>`, such as `int16_t`.

**Definition:**

C++ does not specify the sizes of its integer types. Typically people
assume that `short` is 16 bits, `int` is 32 bits, `long` is 32 bits and
`long long` is 64 bits.

**Pros:**

Uniformity of declaration.

**Cons:**

The sizes of integral types in C++ can vary based on compiler and
architecture.

**Decision:**

`<cstdint>` defines types like `int16_t`, `uint32_t`, `int64_t`, etc.
You should always use those in preference to `short`,
`unsigned long long` and the like, when you need a guarantee on the size
of an integer. When appropriate, you are welcome to use standard types
like `size_t` and `ptrdiff_t`.

For integers we know can be "big", use `int64_t`.

### 64-bit Portability

Code should be 64-bit and 32-bit friendly. Bear in mind problems of
printing, comparisons, and structure alignment.

- `printf()` specifiers for some types are not cleanly portable between
  32-bit and 64-bit systems. C99 defines some portable format
  specifiers. Unfortunately, MSVC 7.1 does not understand some of these
  specifiers and the standard is missing a few, so we have to define our
  own ugly versions in some cases (in the style of the standard include
  file `inttypes.h`):

  ```c++
  // printf macros for size_t, in the style of inttypes.h
  #ifdef _LP64
  #define __PRIS_PREFIX "z"
  #else
  #define __PRIS_PREFIX
  #endif

  // Use these macros after a % in a printf format string
  // to get correct 32/64 bit behavior, like this:
  // size_t size = records.size();
  // printf("%"PRIuS"\n", size);

  #define PRIdS __PRIS_PREFIX "d"
  #define PRIxS __PRIS_PREFIX "x"
  #define PRIuS __PRIS_PREFIX "u"
  #define PRIXS __PRIS_PREFIX "X"
  #define PRIoS __PRIS_PREFIX "o"
  ```

  | Type                     | DO NOT use            | DO use                   | Notes               |
  | ------------------------ | --------------------- | ------------------------ | ------------------- |
  | `void*` (or any pointer) | `%lx`                 | `%p`                     |                     |
  | `int64_t`                | `%qd`, `%lld`         | `%"PRId64"`              |                     |
  | `uint64_t`               | `%qu`, `%llu`, `%llx` | `%"PRIu64"`, `%"PRIx64"` |                     |
  | `size_t`                 | `%u`                  | `%"PRIuS"`, `%"PRIxS"`   | C99 specifies `%zu` |
  | `ptrdiff_t`              | `%d`                  | `%"PRIdS"`               | C99 specifies `%zd` |

  Note that the `PRI*` macros expand to independent strings which are
  concatenated by the compiler. Hence if you are using a non-constant
  format string, you need to insert the value of the macro into the
  format, rather than the name. It is still possible, as usual, to
  include length specifiers, etc., after the `%` when using the `PRI*`
  macros. So, e.g. `printf("x = %30"PRIuS"\n", x)` would expand on
  32-bit Linux to `printf("x = %30" "u" "\n", x)`, which the compiler
  will treat as `printf("x = %30u\n", x)`.

- Remember that `sizeof(void*)` != `sizeof(int)`. Use `intptr_t` if you
  want a pointer-sized integer.

- You may need to be careful with structure alignments, particularly for
  structures being stored on disk. Any class/structure with a
  `int64_t`/`uint64_t` member will by default end up being 8-byte
  aligned on a 64-bit system. If you have such structures being shared
  on disk between 32-bit and 64-bit code, you will need to ensure that
  they are packed the same on both architectures. Most compilers offer a
  way to alter structure alignment. For gcc, you can use
  `__attribute__((packed))`. MSVC offers `#pragma pack()` and
  `__declspec(align())`.

- Use the `LL` or `ULL` suffixes as needed to create 64-bit constants.
  For example:

  ```c++
  int64_t my_value = 0x123456789LL;
  uint64_t my_mask = 3ULL << 48;
  ```

- If you really need different code on 32-bit and 64-bit systems, use
  `#ifdef _LP64` to choose between the code variants. (But please avoid
  this if possible, and keep any such changes localized.)

### Preprocessor Macros

Be very cautious with macros. Prefer inline functions, enums, and
`const` variables to macros.

Macros mean that the code you see is not the same as the code the
compiler sees. This can introduce unexpected behavior, especially since
macros have global scope.

Luckily, macros are not nearly as necessary in C++ as they are in C.
Instead of using a macro to inline performance-critical code, use an
inline function. Instead of using a macro to store a constant, use a
`const` variable. Instead of using a macro to "abbreviate" a long
variable name, use a reference. Instead of using a macro to
conditionally compile code ... well, don't do that at all (except, of
course, for the `#define` guards to prevent double inclusion of header
files). It makes testing much more difficult.

Macros can do things these other techniques cannot, and you do see them
in the codebase, especially in the lower-level libraries. And some of
their special features (like stringifying, concatenation, and so forth)
are not available through the language proper. But before using a macro,
consider carefully whether there's a non-macro way to achieve the same
result.

The following usage pattern will avoid many problems with macros; if you
use macros, follow it whenever possible:

- Don't define macros in a `.h` file.
- `#define` macros right before you use them, and `#undef` them right
  after.
- Do not just `#undef` an existing macro before replacing it with your
  own; instead, pick a name that's likely to be unique.
- Try not to use macros that expand to unbalanced C++ constructs, or at
  least document that behavior well.
- Prefer not using `##` to generate function/class/variable names.

### 0 and NULL

Use `0` for integers, `0.0` for reals, `nullptr` for pointers, and
`'\0'` for chars.

Use `0` for integers and `0.0` for reals. This is not controversial.

For pointers (address values), C++11 added the `nullptr` construct. This
allows the compiler to do additional checks, and is the preferred NULL
pointer value.

Use `'\0'` for chars. This is the correct type and also makes code more
readable.

### sizeof

Use `sizeof(varname)` instead of `sizeof(type)` whenever possible.

Use `sizeof(varname)` because it will update appropriately if the type
of the variable changes. `sizeof(type)` may make sense in some cases,
but should generally be avoided because it can fall out of sync if the
variable's type changes.

```c++
Struct data;
memset(&data, 0, sizeof(data));
```

<!-- badcode -->

```c++
memset(&data, 0, sizeof(Struct));
```

### C++11

Use C++11 features wherever appropriate.

**Definition:**

C++11 is the current ISO C++ standard. It contains [significant
changes](https://en.wikipedia.org/wiki/C%2B%2B11) both to the language
and libraries from the older standard.

## Naming

The most important consistency rules are those that govern naming. The
style of a name immediately informs us what sort of thing the named
entity is: a type, a variable, a function, a constant, a macro, etc.,
without requiring us to search for the declaration of that entity. The
pattern-matching engine in our brains relies a great deal on these
naming rules.

Naming rules are pretty arbitrary, but we feel that consistency is more
important than individual preferences in this area, so regardless of
whether you find them sensible or not, the rules are the rules.

### General Naming Rules

Function names, variable names, and filenames should be descriptive;
eschew abbreviation. Types and variables should be nouns, while
functions should be "command" verbs.

**How to Name**

Give as descriptive a name as possible, within reason. Do not worry
about saving horizontal space as it is far more important to make your
code immediately understandable by a new reader. Examples of well-chosen
names:

```c++
int num_errors;                  // Good.
int num_completed_connections;   // Good.
```

Poorly-chosen names use ambiguous abbreviations or arbitrary characters
that do not convey meaning:

<!-- badcode -->

```c++
int n;                           // Bad - meaningless.
int nerr;                        // Bad - ambiguous abbreviation.
int n_comp_conns;                // Bad - ambiguous abbreviation.
```

Type and variable names should typically be nouns: e.g., `FileOpener`,
`num_errors`.

Function names should typically be imperative (that is they should be
commands): e.g., `open_file()`, `set_num_errors()`. There is an
exception for accessors, which, described more completely in [Function
Names](#function-names), should be named the same as the variable they
access.

**Abbreviations**

Do not use abbreviations unless they are extremely well known outside
your project. For example:

```c++
// Good
// These show proper names with no abbreviations.
int num_dns_connections;  // Most people know what "DNS" stands for.
int price_count_reader;   // OK, price count. Makes sense.
```

<!-- badcode -->

```c++
// Bad!
// Abbreviations can be confusing or ambiguous outside a small group.
int wgc_connections;  // Only your group knows what this stands for.
int pc_reader;        // Lots of things can be abbreviated "pc".
```

Never abbreviate by leaving out letters:

```c++
int error_count;  // Good.
```

<!-- badcode -->

```c++
int error_cnt;    // Bad.
```

### File Names

Filenames should be all lowercase and can include underscores (`_`) or
dashes (`-`). Follow the convention that your project uses. If there is
no consistent local pattern to follow, prefer "\_".

Examples of acceptable file names:

` my_useful_class.cpp`
`my-useful-class.cpp`
`myusefulclass.cpp`
`test_myusefulclass.cpp // _unittest and _regtest are deprecated.`

C++ files should end in `.cpp` and header files should end in `.h`.

Do not use filenames that already exist in `/usr/include`, such as
`db.h`.

In general, make your filenames very specific. For example, use
`http_server_logs.h` rather than `logs.h`. A very common case is to have
a pair of files called, e.g., `foo_bar.h` and `foo_bar.cpp`, defining a
class called `FooBar`.

Inline functions must be in a `.h` file. If your inline functions are
very short, they should go directly into your `.h` file. However, if
your inline functions include a lot of code, they may go into a third
file that ends in `-inl.h`. In a class with a lot of inline code, your
class could have three files:

```c++
url_table.h      // The class declaration.
url_table.cpp     // The class definition.
url_table-inl.h  // Inline functions that include lots of code.
```

See also the section [-inl.h Files](the-inl-h-files)

### Type Names

Type names start with a capital letter and have a capital letter for
each new word, with no underscores: `MyExcitingClass`, `MyExcitingEnum`.

The names of all types --- classes, structs, typedefs, and enums ---
have the same naming convention. Type names should start with a capital
letter and have a capital letter for each new word. No underscores. For
example:

```c++
// classes and structs
class UrlTable  ...
class UrlTableTester  ...
struct UrlTableProperties  ...

// typedefs
typedef hash_map<UrlTableProperties*, string> PropertiesMap;

// enums
enum UrlTableErrors ...
```

### Variable Names

Variable names are all lowercase, with underscores between words. Class
member variables follow this convention. For instance:
`my_exciting_local_variable`, `my_exciting_member_variable`.

**Common Variable names**

For example:

```c++
string table_name;  // OK - uses underscore.
string tablename;   // OK - all lowercase.
```

<!-- badcode -->

```c++
string tableName;   // Bad - mixed case.
```

**Class Data Members**

Data members (also called instance variables or member variables) are
lowercase with optional underscores like regular variable names.

```c++
string table_name;  // OK - underscore at end.
string tablename;   // OK.
```

**Struct Variables**

Data members in structs should be named like regular variables.

```c++
struct UrlTableProperties
{
    string name;
    int num_entries;
}
```

See [Structs vs. Classes](#structs-vs-classes) for a discussion of when
to use a struct versus a class.

**Global Variables**

There are no special requirements for global variables, which should be
rare in any case, but if you use one, consider prefixing it with `g_` or
some other marker to easily distinguish it from local variables.

### Constant Names

Name constants like other variables, using all lowercase, with
underscores between words. `default_width`.

It is distracting that whether a variable is "const" affects the name.

```c++
auto const match = map.find(value);
int const width{1024};
int height{900};
```

### Function Names

Regular functions, accessors, and mutators are all lowercase, with
underscores between words.

**Regular Functions**

Functions and accessors are all lowercase, with underscores between
words.

If your function crashes upon an error, you should append \_or_die to
the function name. This only applies to functions which could be used by
production code and to errors that are reasonably likely to occur during
normal operation.

```c++
add_table_entry()
delete_url()
open_file_or_die()
```

**Accessors and Mutators**

Accessors and mutators (get and set functions) should follow the naming
conventions for regular functions.

```c++
class MyClass
{
public:
    ...
    int get_num_entries() const { return num_entries; }
    void set_num_entries(int value) { num_entries = value; }

private:
    int num_entries;
};
```

### Namespace Names

Namespace names are all lower-case, and based on project names and
possibly their directory structure: `my_awesome_project`.

See [Namespaces](#namespaces) for a discussion of namespaces and how to
name them.

### Enumerator Names

Enumerators should be named like [member
variables](#class-data-members): `out_of_memory`, enclosed within an
enum class.

Preferably, the individual enumerators should be named like [class data
members](#class-data-members). The enumeration name, `UrlTableErrors`,
is a type, and therefore mixed case.

```c++
enum class UrlTableErrors
{
    ok,
    out_of_memory,
    malformed_input,
};
```

### Macro Names

You're not really going to [define a macro](#preprocessor-macros), are
you? If you do, they're like this:
`MY_MACRO_THAT_SCARES_SMALL_CHILDREN`.

Please see the [description of macros](#preprocessor-macros); in general
macros should *not* be used. However, if they are absolutely needed,
then they should be named with all capitals and underscores.

```c++
#define ROUND(x) ...
#define PI_ROUNDED 3.0
```

### Exceptions to Naming Rules

If you are naming something that is analogous to an existing C or C++
entity then you can follow the existing naming convention scheme.

`bigopen()`
: function name, follows form of `open()`

`uint`
: `typedef`

`bigpos`
: `struct` or `class`, follows form of `pos`

`sparse_hash_map`
: STL-like entity; follows STL naming conventions

`LONGLONG_MAX`
: a constant, as in `INT_MAX`

## Comments

Though a pain to write, comments are absolutely vital to keeping our
code readable. The following rules describe what you should comment and
where. But remember: while comments are very important, the best code is
self-documenting. Giving sensible names to types and variables is much
better than using obscure names that you must then explain through
comments.

When writing your comments, write for your audience: the next
contributor who will need to understand your code. Be generous --- the
next one may be you!

### Comment Style

Use either the `//` or `/* */` syntax, as long as you are consistent.

You can use either the `//` or the `/* */` syntax; however, `//` is
*much* more common. Be consistent with how you comment and what style
you use where.

### File Comments

Start each file with a copyright notice, followed by a description of
the contents of the file.

**Legal Notice and Author Line**

Every file should contain the following items, in order:

- a optional mode line,
  `// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-`
- a copyright statement (for example,
  `Copyright (C) 2011 Canonical Ltd`)
- the license boilerplate.
- an author line to identify the original author of the file

If you make significant changes to a file that someone else originally
wrote, add yourself to the author line. This can be very helpful when
another contributor has questions about the file and needs to know whom
to contact about it.

**File Contents**

Every file should have a comment at the top, below the copyright notice
and author line, that describes the contents of the file.

Generally a `.h` file will describe the classes that are declared in the
file with an overview of what they are for and how they are used. A
`.cpp` file should contain more information about implementation details
or discussions of tricky algorithms. If you feel the implementation
details or a discussion of the algorithms would be useful for someone
reading the `.h`, feel free to put it there instead, but mention in the
`.cpp` that the documentation is in the `.h` file.

Do not duplicate comments in both the `.h` and the `.cpp`. Duplicated
comments diverge.

### Class Comments

Every class definition should have an accompanying comment that
describes what it is for and how it should be used.

```c++
// Iterates over the contents of a GargantuanTable.  Sample usage:
//    GargantuanTableIterator* iter = table->new_iterator();
//    for (iter->seek("foo"); !iter->done(); iter->next())
//    {
//        process(iter->key(), iter->value());
//    }
//    delete iter;
class GargantuanTableIterator
{
  ...
};
```

If you have already described a class in detail in the comments at the
top of your file feel free to simply state "See comment at top of file
for a complete description", but be sure to have some sort of comment.

Document the synchronization assumptions the class makes, if any. If an
instance of the class can be accessed by multiple threads, take extra
care to document the rules and invariants surrounding multithreaded use.

### Function Comments

Declaration comments describe use of the function; comments at the
definition of a function describe operation.

**Function Declarations**

Every function declaration should have comments immediately preceding it
that describe what the function does and how to use it. These comments
should be descriptive ("Opens the file") rather than imperative
("Open the file"); the comment describes the function, it does not
tell the function what to do. In general, these comments do not describe
how the function performs its task. Instead, that should be left to
comments in the function definition.

Types of things to mention in comments at the function declaration:

- What the inputs and outputs are.
- For class member functions: whether the object remembers reference
  arguments beyond the duration of the method call, and whether it will
  free them or not.
- If the function allocates memory that the caller must free.
- Whether any of the arguments can be `NULL`.
- If there are any performance implications of how a function is used.
- If the function is re-entrant. What are its synchronization
  assumptions?

Here is an example:

```c++
// Returns an iterator for this table.  It is the client's
// responsibility to delete the iterator when it is done with it,
// and it must not use the iterator once the GargantuanTable object
// on which the iterator was created has been deleted.
//
// The iterator is initially positioned at the beginning of the table.
//
// This method is equivalent to:
//    Iterator* iter = table->new_iterator();
//    iter->seek("");
//    return iter;
// If you are going to immediately seek to another place in the
// returned iterator, it will be faster to use new_iterator()
// and avoid the extra seek.
Iterator* get_iterator() const;
```

However, do not be unnecessarily verbose or state the completely
obvious. Notice below that it is not necessary to say "returns false
otherwise" because this is implied.

```c++
// Returns true if the table cannot hold any more entries.
bool is_table_full();
```

When commenting constructors and destructors, remember that the person
reading your code knows what constructors and destructors are for, so
comments that just say something like "destroys this object" are not
useful. Document what constructors do with their arguments (for example,
if they take ownership of pointers), and what cleanup the destructor
does. If this is trivial, just skip the comment. It is quite common for
destructors not to have a header comment.

**Function Definitions**

Each function definition should have a comment describing what the
function does if there's anything tricky about how it does its job. For
example, in the definition comment you might describe any coding tricks
you use, give an overview of the steps you go through, or explain why
you chose to implement the function in the way you did rather than using
a viable alternative. For instance, you might mention why it must
acquire a lock for the first half of the function but why it is not
needed for the second half.

Note you should *not* just repeat the comments given with the function
declaration, in the `.h` file or wherever. It's okay to recapitulate
briefly what the function does, but the focus of the comments should be
on how it does it.

### Variable Comments

In general the actual name of the variable should be descriptive enough
to give a good idea of what the variable is used for. In certain cases,
more comments are required.

{#class-data-members}
**Class Data Members**

Each class data member (also called an instance variable or member
variable) should have a comment describing what it is used for. If the
variable can take sentinel values with special meanings, such as `NULL`
or -1, document this. For example:

```c++
private:
    // Keeps track of the total number of entries in the table.
    // Used to ensure we do not go over the limit. -1 means
    // that we don't yet know how many entries the table has.
    int num_total_entries;
```

**Global Variables**

As with data members, all global variables should have a comment
describing what they are and what they are used for. For example:

```c++
// The total number of tests cases that we run through in this regression test.
const int number_of_test_cases = 6;
```

### Implementation Comments

In your implementation you should have comments in tricky, non-obvious,
interesting, or important parts of your code.

**Class Data Members**

Tricky or complicated code blocks should have comments before them.
Example:

```c++
// Divide result by two, taking into account that x
// contains the carry from the add.
for (int i = 0; i < result->size(); i++)
{
    x = (x << 8) + (*result)[i];
    (*result)[i] = x >> 1;
    x &= 1;
}
```

**Line Comments**

Also, lines that are non-obvious should get a comment at the end of the
line. These end-of-line comments should be separated from the code by 2
spaces. Example:

```c++
// If we have enough memory, mmap the data portion too.
mmap_budget = max<int64>(0, mmap_budget - index_->length());
if (mmap_budget >= data_size_ && !mmap_data(mmap_chunk_bytes, mlock))
    return;  // Error already logged.
```

Note that there are both comments that describe what the code is doing,
and comments that mention that an error has already been logged when the
function returns.

If you have several comments on subsequent lines, it can often be more
readable to line them up:

```c++
do_something();                      // Comment here so the comments line up.
do_Something_else_that_is_longer();  // Comment here so there are two spaces between
                                     // the code and the comment.
{   // Three space before comment when opening a new scope is allowed,
    // thus the comment lines up with the following comments and code.
    do_something_else();  // Two spaces before line comments normally.
}
```

**nullptr, true/false, 1, 2, 3...**

When you pass in `nullptr`, boolean, or literal integer values to
functions, you should consider adding a comment about what they are, or
make your code self-documenting by using constants. For example,
compare:

<!-- badcode -->

```c++
bool success = calculate_something(interesting_value,
                                   10,
                                   false,
                                   nullptr);  // What are these arguments??
```

versus:

```c++
bool success = calculate_something(interesting_value,
                                   10,        // Default base value.
                                   false,     // Not the first time we're calling this.
                                   nullptr);  // No callback.
```

Or alternatively, constants or self-describing variables:

```c++
int const default_base_value = 10;
bool const first_time_calling = false;
Callback* null_callback = nullptr;
bool success = calculate_something(interesting_value,
                                   default_base_value,
                                   first_time_calling,
                                   null_callback);
```

**Don'ts**

Note that you should *never* describe the code itself. Assume that the
person reading the code knows C++ better than you do, even though he or
she does not know what you are trying to do:

<!-- badcode -->

```c++
// Now go through the b array and make sure that if i occurs,
// the next element is i+1.
...        // Geez.  What a useless comment.
```

### Punctuation, Spelling and Grammar

Pay attention to punctuation, spelling, and grammar; it is easier to
read well-written comments than badly written ones.

Comments should usually be written as complete sentences with proper
capitalization and periods at the end. Shorter comments, such as
comments at the end of a line of code, can sometimes be less formal, but
you should be consistent with your style. Complete sentences are more
readable, and they provide some assurance that the comment is complete
and not an unfinished thought.

Although it can be frustrating to have a code reviewer point out that
you are using a comma when you should be using a semicolon, it is very
important that source code maintain a high level of clarity and
readability. Proper punctuation, spelling, and grammar help with that
goal.

### TODO Comments

Use `TODO` comments for code that is temporary, a short-term solution,
or good-enough but not perfect.

`TODO`s should include the string `TODO` in all caps, followed by the
name, e-mail address, or other identifier of the person who can best
provide context about the problem referenced by the `TODO`. A colon is
optional. The main purpose is to have a consistent `TODO` format that
can be searched to find the person who can provide more details upon
request. A `TODO` is not a commitment that the person referenced will
fix the problem. Thus when you create a `TODO`, it is almost always your
name that is given.

```c++
// TODO(kl@gmail.com): Use a "*" here for concatenation operator.
// TODO(Zeke) change this to use relations.
```

If your `TODO` is of the form "At a future date do something" make
sure that you either include a very specific date ("Fix by November
2005") or a very specific event ("Remove this code when all clients
can handle XML responses.").

### Deprecation Comments

Mark deprecated interface points with `DEPRECATED` comments.

You can mark an interface as deprecated by writing a comment containing
the word `DEPRECATED` in all caps. The comment goes either before the
declaration of the interface or on the same line as the declaration.

After the word `DEPRECATED`, write your name, e-mail address, or other
identifier in parentheses.

A deprecation comment must include simple, clear directions for people
to fix their callsites. In C++, you can implement a deprecated function
as an inline function that calls the new interface point.

Marking an interface point `DEPRECATED` will not magically cause any
callsites to change. If you want people to actually stop using the
deprecated facility, you will have to fix the callsites yourself or
recruit a crew to help you.

New code should not contain calls to deprecated interface points. Use
the new interface point instead. If you cannot understand the
directions, find the person who created the deprecation and ask them for
help using the new interface point.

## Formatting

Coding style and formatting are pretty arbitrary, but a project is much
easier to follow if everyone uses the same style. Individuals may not
agree with every aspect of the formatting rules, and some of the rules
may take some getting used to, but it is important that all project
contributors follow the style rules so that they can all read and
understand everyone's code easily.

### Line Length

Each line of text in your code should be at most 120 characters long.

We recognize that this rule is controversial, with so much existing code
already adhering to a maximum line length of 80 characters. However, we
favor sensible naming of variables and functions over the limit of 80
characters.

**Pros:**

Those who favor this rule argue that it is rude to force them to resize
their windows and there is no need for anything longer. Some folks are
used to having several code windows side-by-side, and thus don't have
room to widen their windows in any case. People set up their work
environment assuming a particular maximum window width, and 80 columns
has been the traditional standard. Why change it?

**Cons:**

Proponents of change argue that a wider line can make code more
readable. The 80-column limit is an hidebound throwback to 1960s
mainframes; modern equipment has wide screens that can easily show
longer lines.

**Decision:**

120 characters is the maximum.

### Non-ASCII Characters

Non-ASCII characters should be rare, and must use UTF-8 formatting.

You shouldn't hard-code user-facing text in source, even English, so
use of non-ASCII characters should be rare. However, in certain cases it
is appropriate to include such words in your code. For example, if your
code parses data files from foreign sources, it may be appropriate to
hard-code the non-ASCII string(s) used in those data files as
delimiters. More commonly, unit test code (which does not need to be
localized) might contain non-ASCII strings. In such cases, you should
use UTF-8, since that is an encoding understood by most tools able to
handle more than just ASCII. Hex encoding is also OK, and encouraged
where it enhances readability --- for example, `"\xEF\xBB\xBF"` is the
Unicode zero-width no-break space character, which would be invisible if
included in the source as straight UTF-8.

### Spaces vs. Tabs

Use only spaces, and indent 4 spaces at a time.

We use spaces for indentation. Do not use tabs in your code. You should
set your editor to emit spaces when you hit the tab key.

### Function Declarations and Definitions

`void` or `auto` on the same line as function name, parameters and
return type on the same line if they fit.

Functions look like this:

```c++
auto ClassName::function_name(Type par_name1, Type par_name2) -> ReturnType
{
    do_something();
    ...
}
```

or:

```c++
auto ClassName::long_function_name(Type par_name1, Type par_name2, Type par_name3)
-> ReturnType
{
    do_something();
    ...
}
```

or:

```c++
auto ClassName::really_really_really_long_function_name(
    Type par_name1,
    Type par_name2,
    Type par_name3) -> ReturnType
{
    do_something();
    ...
}
```

Some points to note:

- `void` or `auto` is always on the same line as the function name.
- The open parenthesis is always on the same line as the function name.
- There is never a space between the function name and the open
  parenthesis.
- There is never a space between the parentheses and the parameters.
- The open curly brace is always on the line following the last
  parameter or return type.
- The close curly brace is either on the last line by itself or (if
  other style rules permit) on the same line as the open curly brace.
- All parameters should be named, with identical names in the
  declaration and implementation. (Except where unused parameters are
  suppressed in the implementation.)
- All parameters should be in a single line if possible, otherwise:
  - Place each parameter in a separate line and indent each with 4
    spaces.
  - Wrap groups of parameters into the next line, and indent each with 4
    spaces.

If your function is `const`, the `const` keyword should be on the same
line as the last parameter:

```c++
// Everything in this function signature fits on a single line
auto function_name(Type par) const -> ReturnType
{
    ...
}

// This function signature requires multiple lines, but
// the const keyword is on the line with the last parameter.
void really_long_function_name(
    Type par1,
    Type par2) const
{
    ...
}
```

If some parameters are unused, comment out the variable name in the
function definition:

```c++
// Always have named parameters in interfaces.
class Shape
{
public:
    virtual void rotate(double radians) = 0;
}

// Always have named parameters in the declaration.
class Circle : public Shape
{
public:
    virtual void rotate(double radians);
}

// Comment out unused named parameters in definitions.
void Circle::rotate(double /*radians*/) {}
```

<!-- badcode -->

```c++
// Bad - if someone wants to implement later, it's not clear what the
// variable means.
void Circle::rotate(double) {}
```

### Function Calls

On one line if it fits; otherwise, wrap arguments at the parenthesis.

Function calls have the following format:

```c++
bool retval = do_something(argument1, argument2, argument3);
```

If the arguments do not all fit on one line, they should be broken up
onto multiple lines, with each subsequent line indented four spaced. Do
not add spaces after the open paren or before the close paren:

```c++
bool retval = do_something(
    averyveryveryverylongargument1,
    argument2, argument3);
```

If the function has many arguments, consider having one per line if this
makes the code more readable:

```c++
bool retval = do_something(
    argument1,
    argument2,
    argument3,
    argument4);
```

### Conditionals

Prefer no spaces inside parentheses. The `else` keyword belongs on a new
line.

```c++
if (condition)  // no spaces inside parentheses
{
    ...  // 4 space indent.
}
else  // The else goes on a new line.
{
    ...
}
```

Note that in all cases you must have a space between the `if` and the
open parenthesis.

<!-- badcode -->

```c++
if(condition)     // Bad - space missing after IF.
```

```c++
if (condition)   // Good - proper space after IF.
```

Short conditional statements may be written on one line if this enhances
readability. You may use this only when the line is brief and the
statement does not use the `else` clause.

```c++
if (x == foo) return new Foo();
if (x == bar) return new Bar();
```

This is not allowed when the if statement has an `else`:

<!-- badcode -->

```c++
// Not allowed - IF statement on one line when there is an ELSE clause
if (x) do_this();
else do_that();
```

Curly braces are preferred if the statement is on a different line than
the condition.

```c++
if (condition)
{
    do_something();  // 4 space indent.
}
```

<!-- badcode -->

```c++
// Not allowed - single line IF statements without curly braces
if (condition)
    foo;

if (condition)
    foo;
else
    bar;
```

### Loops and Switch Statements

Switch statements may use braces for blocks. Empty loop bodies should
use `{}` or `continue`.

`case` blocks in `switch` statements can have curly braces or not,
depending on your preference. If you do include curly braces they should
be placed as shown below.

If not conditional on an enumerated value, switch statements should
always have a `default` case (in the case of an enumerated value, the
compiler will warn you if any values are not handled). If the default
case should never execute, simply `assert`:

```c++
switch (var)
{
case 0:      // no indent
    ...      // 4 space indent
    break;
case 1:
    {
        ...
        break;
    }
default:
    assert(false);
}
```

Empty loop bodies should use `{}` or `continue`, but not a single
semicolon.

```c++
while (condition)
{
    // Repeat test until it returns false.
}
for (int i = 0; i < some_number_with_descriptive_name; ++i) {}  // Good - empty body.
while (condition) continue;  // Good - continue indicates no logic.
```

<!-- badcode -->

```c++
while (condition);  // Bad - looks like part of do/while loop.
```

### Pointer and Reference Expressions

No spaces around period or arrow. Pointer operators do not have trailing
spaces.

The following are examples of correctly-formatted pointer and reference
expressions:

```c++
x = *p;
p = &x;
x = r.y;
x = r->y;
```

Note that:

- There are no spaces around the period or arrow when accessing a
  member.
- Pointer operators have no space after the `*` or `&`.

When declaring a pointer variable or argument, you should place the
asterisk adjacent to the type:

```c++
// These are fine
char* c;
string const& str;
```

<!-- badcode -->

```c++
char * c;  // Bad - spaces on both sides of *
char *c ;  // Bad - * next to variable name
string const & str;  // Bad - spaces on both sides of &
```

You should do this consistently within a single file, so, when modifying
an existing file, use the style in that file.

### Boolean Expressions

When you have a boolean expression that is longer than the [standard
line length](#line-length), be consistent in how you break up the lines.

In this example, the logical AND operator is always at the end of the
lines:

```c++
if (this_one_thing > this_other_thing &&
    a_third_thing == a_fourth_thing &&
    yet_another && last_one)
{
    ...
}
```

Note that when the code wraps in this example, both of the `&&` logical
AND operators are at the end of the line. Feel free to insert extra
parentheses judiciously, because they can be very helpful in increasing
readability when used appropriately. Also note that you should always
use the punctuation operators, such as `&&` and `~`, rather than the
word operators, such as `and` and `compl`.

### Return Values

Do not needlessly surround the `return` expression with parentheses.

Use parentheses in `return expr;` only where you would use them in
`x = expr;`.

```c++
return result;                  // No parentheses in the simple case.
return (some_long_condition &&  // Parentheses ok to make a complex
        another_condition);     //     expression more readable.
```

<!-- badcode -->

```c++
return (value);                // You wouldn't write var = (value);
return(result);                // return is not a function!
```

### Variable and Array Initialization

Use `{}`.

You may choose between `=` and `()`; the following are all correct:

```c++
 //C++11: default initialization using {}
    int n{5}; //zero initialization: n is initialized to 5
    int* p{}; //initialized to nullptr
    double d{}; //initialized to 0.0
    char s[12]{}; //all 12 chars are initialized to '\0'
    string s{}; //same as: string s;
    string name{"Some Name"};
    char* p=new char [5]{}; // all five chars are initialized to '\0'
```

### Preprocessor Directives

The hash mark that starts a preprocessor directive should always be at
the beginning of the line.

Even when preprocessor directives are within the body of indented code,
the directives should start at the beginning of the line.

```c++
// Good - directives at beginning of line
if (lopsided_score)
{
#if DISASTER_PENDING      // Correct -- Starts at beginning of line
    drop_everything();
# if NOTIFY               // OK but not required -- Spaces after #
    notify_client();
# endif
#endif
    back_to_normal();
}
```

<!-- badcode -->

```c++
// Bad - indented directives
if (lopsided_score)
{
    #if DISASTER_PENDING  // Wrong!  The "#if" should be at beginning of line
    drop_everything();
    #endif                // Wrong!  Do not indent "#endif"
    back_to_normal();
}
```

### Class Format

Sections in `public`, `protected` and `private` order.

The basic format for a class declaration (lacking the comments, see
[Class Comments](#class-comments) for a discussion of what comments are
needed) is:

```c++
      class MyClass : public OtherClass
      {
      public:
          MyClass();  // Regular 4 space indent.
          explicit MyClass(int var);
          ~MyClass() {}

          void some_function();
          void some_function_that_does_nothing() {}

          void set_some_var(int var) { some_var_ = var; }
          int some_var() const { return some_var_; }

      private:
          MyClass(MyClass const&) = delete;
          MyClass& operator=(MyClass const&) = delete;
          bool some_internal_function();

          int some_var_;
          int some_other_var;
      };
```

Things to note:

- Any base class name should be on the same line as the subclass name,
  subject to the 80-column limit.
- The `public:`, `protected:`, and `private:` keywords are not indented.
- Except for the first instance, these keywords should be preceded by a
  blank line. This rule is optional in small classes.
- Do not leave a blank line after these keywords.
- The `public` section should be first, followed by the `protected` and
  finally the `private` section.
- See [Declaration Order](#declaration-order) for rules on ordering
  declarations within each of these sections.

### Constructor Initializer Lists

Constructor initializer lists can be all on one line or with subsequent
lines indented four spaces.

There are three acceptable formats for initializer lists:

```c++
// When it all fits on one line:
MyClass::MyClass(int var) : some_var_(var), some_other_var_(var + 1) {}
```

or

```c++
// When it requires multiple lines, indent 4 spaces, putting the colon on
// the constructor declaration line and commas at the end of the line.
MyClass::MyClass(int var) :
    some_var_(var),            // 4 space indent
    some_other_var_(var + 1)
{
    ...
    do_something();
    ...
}
```

or

```c++
// When it requires multiple lines, indent 4 spaces, putting the colon on
// the first initializer line and commas at the end of the line.
MyClass::MyClass(int var)
    : some_var_(var),            // 4 space indent
      some_other_var_(var + 1)
{
    ...
    do_something();
    ...
}
```

### Namespace Formatting

The contents of namespaces are not indented.

[Namespaces](#namespaces) do not add an extra level of indentation. For
example, use:

```c++
namespace
{

void foo()  // Correct.  No extra indentation within namespace.
{
    ...
}

}  // namespace
```

Do not indent within a namespace:

<!-- badcode -->

```c++
namespace
{

  // Wrong.  Indented when it should not be.
  void foo()
  {
    ...
  }

}  // namespace
```

When declaring nested namespaces, put each namespace on its own line,
with the opening brace on the line following.

```c++
namespace foo
{
namespace bar
{
```

### Horizontal Whitespace

Use of horizontal whitespace depends on location. Never put trailing
whitespace at the end of a line.

**General**

```c++
int i = 0;        // Semicolons usually have no space before them.
int x[] = { 0 };  // Spaces inside braces for array initialization are
int x[] = {0};    // optional.  If you use them, put them on both sides!

// Spaces around the colon in inheritance and initializer lists.
class Foo : public Bar
{
public:
    // For inline function implementations, put spaces between the braces
    // and the implementation itself.
    Foo(int b) : Bar(), baz_(b) {}  // No spaces inside empty braces.
    void reset() { baz_ = 0; }  // Spaces separating braces from implementation.
    ...
```

Adding trailing whitespace can cause extra work for others editing the
same file, when they merge, as can removing existing trailing
whitespace. So: Don't introduce trailing whitespace. Remove it if
you're already changing that line, or do it in a separate clean-up
operation (preferably when no-one else is working on the file).

**Operators**

```c++
x = 0;              // Assignment operators always have spaces around
                    // them.
x = -5;             // No spaces separating unary operators and their
++x;                // arguments.
if (x && !y)
  ...
v = w * x + y / z;  // Binary operators usually have spaces around them,
v = w*x + y/z;      // but it's okay to remove spaces around factors.
v = w * (x + z);    // Parentheses should have no spaces inside them.
```

**Templates and Casts**

```c++
vector<string> x;           // No spaces inside the angle
y = static_cast<char*>(x);  // brackets (< and >), before
                            // <, or between >( in a cast.
vector<char*> x;            // Spaces between type and pointer are
                            // okay, but be consistent.
set<list<string>> x;        // C++11 now allows >> to close templates.
set<list<string> > x;       // Older C++ requiree a space in > >, and is allowed.
```

### Vertical Whitespace

Minimize use of vertical whitespace but apply it to enhance readability.

This is more a principle than a rule: don't use blank lines when you
don't have to. In particular, don't put more than one or two blank
lines between functions, resist starting functions with a blank line,
don't end functions with a blank line, and be discriminating with your
use of blank lines inside functions.

The basic principle is: The more code that fits on one screen, the
easier it is to follow and understand the control flow of the program.
Of course, readability can suffer from code being too dense as well as
too spread out, so use your judgement. But in general, minimize use of
vertical whitespace.

Some rules of thumb to help when blank lines may be useful:

- Blank lines at the beginning or end of a function very rarely help
  readability.
- Blank lines inside a chain of if-else blocks may well help
  readability.

## Exceptions to the Rules

The coding conventions described above are mandatory. However, like all
good rules, these sometimes have exceptions, which we discuss here.

### Existing Non-conformant Code

You may diverge from the rules when dealing with code that does not
conform to this style guide.

If you find yourself modifying code that was written to specifications
other than those presented by this guide, you may have to diverge from
these rules in order to stay consistent with the local conventions in
that code. If you are in doubt about how to do this, ask the original
author or the person currently responsible for the code. Remember that
*consistency* includes local consistency, too.

## Parting Words

Use common sense and *BE CONSISTENT*.

If you are editing code, take a few minutes to look at the code around
you and determine its style. If they use spaces around their `if`
clauses, you should, too. If their comments have little boxes of stars
around them, make your comments have little boxes of stars around them
too.

The point of having style guidelines is to have a common vocabulary of
coding so people can concentrate on what you are saying, rather than on
how you are saying it. We present global style rules here so people know
the vocabulary. But local style is also important. If code you add to a
file looks drastically different from the existing code around it, the
discontinuity throws readers out of their rhythm when they go to read
it. Try to avoid this.

OK, enough writing about writing code; the code itself is much more
interesting. Have fun!
