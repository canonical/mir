Installing pre-built packages on an Android device {#installing_prebuilt_on_android}
==================================================

Supported Devices
-----------------

The following devices have been tested with Mir:

| Device             | Status |
| ------------------ | ------ |
| Galaxy Nexus (SGX) | tested |
| Nexus 7 (nvidia)   | tested - needs special hybris for nvidia atm |
| Nexus 4 (qualcomm) | tested |

Installing Mir
--------------

1. First you must install a phablet image on your device, by following the
   directions at https://wiki.ubuntu.com/Touch/Install on one of the supported
   devices. Once you have booted into the device with Ubuntu Touch ensure you
   then connect the device to the internet.

2. With your device connected via USB, type:

       $ adb devices

   just to ensure you are connected, then open the android shell:

       $ adb root
       $ adb shell

3. Add the Mir staging PPA to your device's sources list:

       # apt-get install software-properties-common
       # add-apt-repository ppa:mir-team/staging
       # apt-get update

4. Install Mir:

       # apt-get install mir-demos
