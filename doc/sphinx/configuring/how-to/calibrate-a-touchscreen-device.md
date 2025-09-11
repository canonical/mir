(how-to-calibrate-a-touchscreen-device)=

# How to calibrate a touchscreen device

## Identifying the touch device

Install the libinput snap:

```shell
sudo snap install libinput
```

Use the libinput snap to identify the device:

```shell
$ sudo libinput.list-devices | grep "Calibration:      [^n][^/][^a]" --before 11 --after 5 | grep -v n/a
Device:           QDtech MPI7003
Kernel:           /dev/input/event12
Group:            7
Seat:             seat0, default
Capabilities:     touch
Calibration:      identity matrix
Scroll methods:   none
Click methods:    none
```

From this take the "Kernel" device path:

```shell
$ udevadm info -q property /dev/input/event21 | grep -e ID_VENDOR_ID -e ID_MODEL_ID
ID_VENDOR_ID=0483
ID_MODEL_ID=5750
```

We will need those IDs for the udev rule. (In this case we will have `ATTRS{idVendor}=="0483",ATTRS{idProduct}=="5750",`.)

## Generating the calibration matrix

Now calibrate the touchscreen by pressing three target spots in turn:

```shell
$ libinput.calibrate-touchscreen
Starting the calibrate-touchscreen app: touch the target spots.
(Or press <ESC> to quit!)
Calibration = 1.008, 0.001, -0.002, -0.012, 1.003, 0.002
```

## Setting the udev rule

We now have the information needed to calibrate the touchscreen:

```shell
echo 'ATTRS{idVendor}=="0483",ATTRS{idProduct}=="5750", ENV{LIBINPUT_CALIBRATION_MATRIX}="0 1.035 -0.021 1.007 0 -0.018"' | \
sudo tee /etc/udev/rules.d/99-QDtech-MPI7003.rules
```
And ensure the new rule has been processed by udev:
```shell
sudo udevadm control --reload
```

## Verifying the updated calibration

Now _replug the touchscreen_ and see the changes:

```shell
$ sudo libinput.list-devices | grep "Calibration:      [^n][^/][^a]" --before 11 --after 5 | grep -v n/a
Device:           QDtech MPI7003
Kernel:           /dev/input/event12
Group:            7
Seat:             seat0, default
Capabilities:     touch
Calibration:      0.00 1.03 -0.02 1.01 0.00 -0.02
Scroll methods:   none
Click methods:    none
```
