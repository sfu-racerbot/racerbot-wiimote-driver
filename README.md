# ROS2 Wiimote Driver
This is a node for ROS2 that allows the use of a Wiimote (Wii Remote) to be used to control your robot. 

The node publishes inputs to the `/joy` ROS topic and uses `sensor_msgs::msg::Joy` as it's message type.

## Features/Roadmap
- [x] Detection of A, B and 2 buttons (for acceleration, braking, and the "deadman" switch respectively)
- [ ] Motion controls mapping to joystick inputs
- [x] LED Status indicators

## Usage

### Pre-Requisties
You need to install `xwiimote`, `libxwiimote-dev`, and `pkg-config` (on Debian based systems; other OSes will have different names for these packages) for this node to work.

Currently only Linux is supported
### Installation
1. Clone this repository into your ROS2 workspace
2. Run:
```bash
rosdep update
rosdep install --from-paths src --ignore-src -r -y
colcon build --packages-select racerbot_wiimote_driver
```
3. To launch the node run:
```bash
ros2 launch racerbot_wiimote_driver wiimote_driver_launch.py
```

### ROS2 Parameters
- `joy_topic` Topic to publish `sensor_msgs/Joy` messages on. (default: `joy`)
- `publisher_queue_depth` QoS queue depth for the joy publisher. (default: `1`)
- `poll_period_ms` How often (ms) to poll the Wiimote for input. (default: `5`)
- `accelerate_button_index` Index in `Joy.buttons` that reflects the accelerate (A) button. (default: `1`)
- `brake_button_index` Index in `Joy.buttons` that reflects the brake (B) button. (default: `2`)
- `deadman_button_index` Index in `Joy.buttons` that reflects the deadman (2) button. (default: `4`)
- `disable_deadman_led` Disables the LED indicator when the deadman switch is engaged. Useful when manually controlling LEDs.

### ROS2 Topics
- `/wiimote` General information about the Wiimote such as battery life. Uses the `racerbot_wii_msgs::msg::WiimoteRaw` message type.
- `/wiimote/led` A topic in which you can publish messages to control the LEDs on the Wiimote. It is highly recommended to pass the ROS2 parameter `disable_deadman_led` when manually controlling the LEDs. Uses the `racerbot_wii_msgs::msg::WiimoteLED`.
- `/wiimote/buttons` Returns which buttons on the Wiimote are being pressed at the current moment in time. Uses the `racerbot_wii_msgs::msg::WiimoteButtonsRaw` message type.

### Wiimote LED Reference
The driver sets the LEDs on the Wiimote to indicate various things to the user. Here is what each LED means, moving from left to right on the Wiimote being held
upright.

- **LED 0 (Player One LED):** Indicates the Wiimote is connected.
- **LED 1 (Player Two LED):** Indicates the "deadman" switch is engaged, meaning your autonomous code will run (if you have a proper deadman switch built into your code)

You can easily change the LED behaviour by publishing `racerbot_wii_msgs::msg::WiimoteLED` messages to `/wiimote/led`.

## Known Issues/Troubleshooting

### Troubleshooting: LED permission denied (error -13)
 
If `set_led()` throws with error code `-13` (`EACCES`), it's a Linux file
permissions issue, not a bug in the driver. The Wiimote's LED brightness
files under `/sys/class/leds/` are owned by `root` and default to
read-only for other users, so writing to them without elevated
permissions fails.
 
**Fix (one-time):**
 
```bash
sudo chmod 666 /sys/class/leds/*:blue:p*/brightness
```
 
**Fix (permanent, recommended):** Create a udev rule so this is applied
automatically every time a Wiimote connects, without needing `sudo`:
 
```bash
sudo tee /etc/udev/rules.d/99-wiimote-leds.rules <<'EOF'
ACTION=="add", SUBSYSTEM=="leds", KERNEL=="*:blue:p[0-3]", RUN+="/bin/chmod 666 /sys/class/leds/%k/brightness"
EOF
 
sudo udevadm control --reload-rules
sudo udevadm trigger
```
 
Then disconnect and reconnect the Wiimote so the rule applies to the new
device.
 
**Verify it's actually a permissions issue:**
 
```bash
ls -l /sys/class/leds/*:blue:p0/brightness
```
 
If the owner/group is `root:root` and the mode doesn't include write
access for your user (e.g. `-rw-r--r--`), that confirms it. You can also
test a direct write to rule out anything in the driver code:
 
```bash
echo 1 | sudo tee /sys/class/leds/*:blue:p0/brightness   # should succeed
echo 1 | tee /sys/class/leds/*:blue:p0/brightness         # fails without the fix above
```
