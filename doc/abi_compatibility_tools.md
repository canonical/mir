Tools to track ABI compatibility {#abi_compatibility_tools}
================================

We have created a few build targets to help us track ABI compatibility across
different Mir versions and ensure we increase the ABI version properly. These
target use the abi-compliance-checker tool to create and check the ABI dumps.

The targets are:

* **make abi-dump** (or abi-dump-<library> for a specific library, e.g., abi-dump-mirclient)

  Produces ABI dumps for the public versioned libraries shipped by Mir. The
  resulting ABI dump for each library is saved as
  <build-dir>/abi_dumps/<arch>/<libname>_next.abi.tar.gz.

  By default, this target also builds the libraries needed for create the
  requested ABI dumps. The environment variable `MIR_ABI_DUMP_PREBUILT_LIBDIR`
  can be set **at configuration time** (i.e., when invoking cmake) to a path
  containing pre-built libraries to use instead.

  _N.B: the path set with MIR_ABI_DUMP_PREBUILT_LIBDIR is expected to contain
  *.so files, not only *.so.X files._

* **make abi-dump-base** (or abi-dump-base-<library> for a specific library,
  e.g., abi-dump-base-mirclient)

  Similar to make abi-dump, but saves the ABI dump as
  <build-dir>/abi_dumps/<arch>/<libname>_base.abi.tar.gz (note the 'base' vs
  'next' in the file name).

* **make abi-check** (or abi-check-<library>, for a specific library)
  e.g., abi-check-mirclient)

  Produces ABI dumps (this target depends on the abi-dump target), and checks
  them against a set of base ABI dumps. The base ABI dumps are assumed to be
  located in <build-dir>/abi_dumps/<arch>/, i.e., where make abi-dump-base
  would normally place the files.

  The environment variable `MIR_ABI_CHECK_BASE_DIR` can be set **at
  configuration time** (i.e., when invoking cmake) to set a different directory
  for the base ABI dumps. The supplied directory must **not** contain the
  <arch> portion of the path (e.g., just build_dir/abi_dumps, not
  build_dir/abi_dumps/x86_64-linux-gnu).

  Since this target depends on the abi-dump target, the
  `MIR_ABI_DUMP_PREBUILT_LIBDIR` environment variable can be set to use
  pre-built libraries instead of building them.

Sample usage
------------

A common scenario is to check the ABI of the development version against the
latest archive version. This is one way to perform the check:

### Step 1: Produce the ABI dumps for the installed version

Start by download the source corresponding to the installed version:

    $ apt-get source mir
    $ cd mir-<version>

At this point we can select whether to build the libraries from scratch:

    $ debian/rules override_dh_auto_configure

... or to reuse the pre-built libraries, assuming they are installed (we
need their -dev packages too!):

    $ MIR_ABI_DUMP_PREBUILT_LIBDIR=/usr/lib/$(dpkg-architecture -qDEB_HOST_MULTIARCH) debian/rules override_dh_auto_configure

Now we can create the ABI dumps:

    $ cd <build-dir> && make abi-dump-base

This will create base ABI dump files which we can use in the next step.

### Step 2: Check the ABI of the development version against the produced base ABI dumps

    $ bzr branch lp:mir && cd mir 
    $ MIR_ABI_CHECK_BASE_DIR=/path/to/base/abi/dumps debian/rules override_dh_auto_configure
    $ cd <build-dir>
    $ make abi-check
