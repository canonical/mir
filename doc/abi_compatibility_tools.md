Tracking ABI compatibility {#abi_compatibility_tools}
================================

A few make targets exist to help us track ABI compatibility across
different Mir versions and ensure we increase the ABI version properly.
These targets invoke the abi-compliance-checker tool for the actual ABI check.

The targets are:

* **make abi-check**

  Compiles all the public libraries in the current tree and checks their ABI against the latest released archive version

* **make abi-check-<library>**

  Compiles only the specified library in the current tree and checks its ABI against the latest released archive version

  - *library* can be any of the public library targets such as mirclient, mirserver, mirplatform, mircommon, etc.

Sample usage
------------

    $ bzr branch lp:mir && cd mir
    $ debian/rules override_dh_auto_configure
    $ cd <build-dir>
    $ make abi-check
