# ROS 2 Wiimote Driver

A ROS 2 driver node that interfaces with a Nintendo Wiimote (Wii Remote) for robot teleoperation and status monitoring.

The node publishes standard joystick messages to `/joy` (`sensor_msgs/msg/Joy`) and provides custom topics for raw Wiimote sensor data and LED control.

---

## Features

* [x] Standard `/joy` teleop mapping (A, B, and 2 buttons mapped to accelerate, brake, and deadman switch).
* [x] Motion control / accelerometer mapping to joystick axes.
* [x] Onboard LED status indicators with customizable overrides.
* [x] Dedicated ROS 2 topics for reading raw battery, accelerometer, and button states.

---

## Prerequisites

> **Note:** Only Linux is currently supported.

If you are using a system with a custom Linux image (such as on an NVIDIA Jetson), follow the steps [here](#wiimote-stays-flashing-when-connected-to-linux-via-bluetooth) first

Install the required system dependencies (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install -y xwiimote libxwiimote-dev pkg-config
```
---

## Installation & Build

1. Clone the repository into the `src` directory of your ROS 2 workspace:
```bash
cd ~/ros2_ws/src
git clone <repository-url>
```


2. Resolve dependencies and build:
```bash
cd ~/ros2_ws
rosdep update
rosdep install --from-paths src --ignore-src -r -y
colcon build --packages-select racerbot_wiimote_driver
source install/setup.bash
```
---
## Usage
Launch the driver node:

```bash
ros2 launch racerbot_wiimote_driver wiimote_driver_launch.py
```
Configurable launch parameters are defined in [`launch/wiimote_driver_launch.py`](src/racerbot_wiimote_driver/launch/wiimote_driver_launch.py).

---
## ROS 2 Interface

### Published Topics

| Topic | Message Type | Description |
| --- | --- | --- |
| `/joy` | `sensor_msgs/msg/Joy` | Standard ROS joystick outputs from Wiimote inputs. |
| `/wiimote/battery` | `racerbot_wii_msgs/msg/WiimoteBattery` | Current Wiimote battery percentage. |
| `/wiimote/buttons` | `racerbot_wii_msgs/msg/WiimoteButtons` | Real-time button press states. |
| `/wiimote/accel` | `racerbot_wii_msgs/msg/WiimoteAccelerometer` | Raw accelerometer readings. |

### Subscribed Topics

| Topic | Message Type | Description |
| --- | --- | --- |
| `/wiimote/led` | `racerbot_wii_msgs/msg/WiimoteLED` | Set Wiimote LED states. *(Pass `disable_deadman_led:=true` parameter when overriding manually).* |

---

## Default LED Indicators

When held upright (left to right):

| LED | Name | Default Function |
| --- | --- | --- |
| **LED 0** | Player 1 | **Connection Active:** Solid when the Wiimote is connected. |
| **LED 1** | Player 2 | **Deadman Switch:** Solid when the deadman switch (Button 2) is engaged. |
| **LED 2** | Player 3 | *Unused (available for manual control).* |
| **LED 3** | Player 4 | *Unused (available for manual control).* |

---

## Troubleshooting

### Wiimote stays flashing when connected to Linux via Bluetooth

This usually means that the `hid-wiimote` driver is not loaded into the Linux kernel. Alot of times custom Linux images (such as those used on NVIDIA Jetsons) strip
out these drivers as they aren't needed.

#### Permanent Fix
You will need to compile the `hid_wiimote` driver from the upstream kernel repository and load it into the kernel.

Install the compiler toolchain (this is for Debian based systems; it will differ slightly on other distros)
```bash
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r) wget
```

Download the driver source code. After it's done make sure to `cat` each file and see it's not a 404 error page.
```bash
mkdir -p ~/hid-wiimote-build && cd ~/hid-wiimote-build

# Set kernel tag based on current running kernel major/minor version (e.g., v6.8)
KERNEL_TAG="v$(uname -r | cut -d'.' -f1,2)"

# Download driver source files and required header dependency
for f in hid-wiimote.h hid-wiimote-core.c hid-wiimote-debug.c hid-wiimote-modules.c hid-ids.h; do
  wget "https://raw.githubusercontent.com/torvalds/linux/${KERNEL_TAG}/drivers/hid/$f"
done
```

Create the Makefile
```bash
cat > Makefile << 'EOF'
obj-m := hid-wiimote.o
hid-wiimote-y := hid-wiimote-core.o hid-wiimote-debug.o hid-wiimote-modules.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
ccflags-y := -I$(PWD)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
EOF
```

Compile the module
```bash
make
```

Load the module into the kernel
```bash
# Load module into kernel
sudo insmod hid-wiimote.ko

# Verify module status
lsmod | grep wiimote
```

Set the module to load on boot
```bash
sudo mkdir -p /lib/modules/$(uname -r)/updates
sudo cp hid-wiimote.ko /lib/modules/$(uname -r)/updates/

sudo depmod -a # update module dependency lists

echo "hid-wiimote" | sudo tee /etc/modules-load.d/hid-wiimote.conf # tell the system to load hid-wiimote at boot

sudo modprobe hid-wiimote # load the hid-wiimote driver
```

### LED Permission Denied (`error -13` / `EACCES`)

If `set_led()` fails with error code `-13`, the Wiimote sysfs files under `/sys/class/leds/` lack write permissions for non-root users.

#### Permanent Fix (Recommended)

Create a `udev` rule to grant permissions automatically upon connection:

```bash
sudo tee /etc/udev/rules.d/99-wiimote-leds.rules <<'EOF'
ACTION=="add", SUBSYSTEM=="leds", KERNEL=="*:blue:p[0-3]", RUN+="/bin/chmod 666 /sys/class/leds/%k/brightness"
EOF

sudo udevadm control --reload-rules
sudo udevadm trigger

```

*Disconnect and reconnect the Wiimote after applying this rule.*

#### Temporary Fix

```bash
sudo chmod 666 /sys/class/leds/*:blue:p*/brightness

```

#### Verifying Permissions

Check the file mode and test direct writes:

```bash
# Check owner and mode (should show read/write for all users: -rw-rw-rw-)
ls -l /sys/class/leds/*:blue:p0/brightness

# Test writing directly without sudo (should succeed without permission errors)
echo 1 | tee /sys/class/leds/*:blue:p0/brightness

```