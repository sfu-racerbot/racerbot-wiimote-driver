# ROS2 Wiimote Driver
This is a node for ROS2 that allows the use of a Wiimote (Wii Remote) to be used to control your robot. 

The node publishes inputs to the `/joy` ROS topic and uses `sensor_msgs::msg::Joy` as it's message type.

## Features/Roadmap
- [x] Detection of A, B and 2 buttons (for acceleration, braking, and the "deadman" switch respectively)
- [ ] Motion controls mapping to joystick inputs
- [ ] LED Status indicators

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