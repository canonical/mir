Installing pre-built packages on a PC {#installing_prebuilt_on_pc}
=====================================

1. Install raring if you haven' t done so already.  Uninstall any proprietary
   drivers (-nvidia, -fglrx) and reboot on the FOSS drivers.

2. Add the ppa:mir-team/staging. Note that besides mir itself, the PPA includes
   custom builds of Mesa and Xorg drivers with support for mir:

       sudo add-apt-repository ppa:mir-team/staging

3. Update your package list:

       sudo apt-get update

4. Create the `/etc/apt/preferences.d/50-pin-mir.pref` file with the following contents:

       Package: *
       Pin: origin "private-ppa.launchpad.net"
       Pin-Priority: 1001

       Package: *
       Pin: release o=LP-PPA-mir-team-staging
       Pin-Priority: 1002

5. Install mir and dist-upgrade:

       sudo apt-get install mir
       sudo apt-get dist-upgrade
