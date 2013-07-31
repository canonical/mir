Installing pre-built packages on a PC {#installing_prebuilt_on_pc}
=====================================

Install saucy if you haven' t done so already.  Uninstall any proprietary
drivers (-nvidia, -fglrx) and reboot on the FOSS drivers.

Add the ppa:mir-team/system-compositor-testing. Note that besides Mir itself,
the PPA includes custom builds of Mesa and Xorg drivers with support for Mir:

       sudo add-apt-repository ppa:mir-team/system-compositor-testing

Update your package list:

       sudo apt-get update

Create the `/etc/apt/preferences.d/50-pin-mir.pref` file with the following
contents:

       Package: *
       Pin: origin "private-ppa.launchpad.net"
       Pin-Priority: 1001

       Package: *
       Pin: release o=LP-PPA-mir-team-system-compositor-testing
       Pin-Priority: 1002

Install Mir and dist-upgrade:

       sudo apt-get install mir-demos
       sudo apt-get dist-upgrade
       sudo apt-get install unity-system-compositor
