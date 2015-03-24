Mir As An Appliance
===================

How to build a Mir-based appliance/kiosk using Ubuntu Core and Snappy
packaging.

Selection of hardware
---------------------

Mir as an appliance presently supports Mesa/DRM drivers. So almost any
hardware that supports Mir on Ubuntu desktop will suffice. Virtual machines
are generally NOT supported by Mir yet. So cannot be used here.

Your chosen machine must be plugged into ethernet by cable (wifi not
supported), and that cable must be able to reach a DHCP server
(e.g. a typical home router). For more details see:
<https://bugs.launchpad.net/snappy-ubuntu/+bugs?field.tag=wontboot>

Getting the operating system
----------------------------

Ubuntu Core is a new product still and effectively pre-release. So you cannot
download usable disk images yet. You need to make your own.

1. Use an Ubuntu desktop running 15.04 or later.
2. Install package: `ubuntu-device-flash`
3. Make a disk image:

        $ sudo ubuntu-device-flash core --enable-ssh -o mysnappy.img

You now have a 20GB (by default) disk image to dump on your machine's drive.
This part and variations on it are left as an exercise for the reader.

Booting the operating system
----------------------------

This can be troublesome. For known issues refer to:
<https://bugs.launchpad.net/snappy-ubuntu/+bugs?field.tag=wontboot>

Once booted you log in as user `ubuntu` with password `ubuntu`.

Learn the basics of snappy
--------------------------

If you haven't already, learn here: <https://developer.ubuntu.com/en/snappy/>

Building snap packages of Mir
-----------------------------

1. Build Mir per normal. But if you want the quickest result then try:

        $ mkdir build   
        $ cd build   
        $ cmake .. -DMIR_ENABLE_TESTS=OFF -DMIR_PLATFORM=mesa   
        $ make -j8

2. Build the .snap files:

        $ make snap

3. Copy the .snap files from the location displayed at the end of step 2.

Running the mir/mir-demos snap packages
---------------------------------------

Once installed you will have two new snappy `/apps`: mir and mir-demos

Presently only server and standalone binaries work. To run them with the
full environment, libraries and drivers they require, use:

    sudo /apps/bin/mir-run /apps/mir-demos/current/mir_proving_server
    sudo /apps/bin/mir-run /apps/mir-demos/current/mir_demo_standalone_render_surfaces
    etc

Automatic setting of privileges to avoid "sudo" is not yet implemented.

