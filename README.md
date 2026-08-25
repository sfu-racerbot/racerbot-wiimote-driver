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

### Wiimote LED Reference
The driver sets the LEDs on the Wiimote to indicate various things to the user. Here is what each LED means, moving from left to right on the Wiimote being held
upright.

- **LED 0 (Player One LED):** Indicates the Wiimote is connected.
- **LED 1 (Player Two LED):** Indicates the "deadman" switch is engaged, meaning your autonomous code will run (if you have a proper deadman switch built into your code)

You can easily change the LED behaviour using the `WiimoteDevice::set_led` function.