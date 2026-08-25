#ifndef WIIMOTE_DRIVER_NODE_HPP_
#define WIIMOTE_DRIVER_NODE_HPP_

#include "racerbot_wiimote_msgs/msg/wiimote_buttons_raw.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joy.hpp"
#include "wiimote_device.hpp"

class WiimoteDriverNode : public rclcpp::Node {
public:
  WiimoteDriverNode();

private:
  // ROS2 Parameters
  int accelerate_button_index_ = 1;
  int brake_button_index_ = 2;
  int deadman_button_index_ = 4;
  size_t num_buttons_ = 5;

  WiimoteDevice device_;

  // Previous button states, used only to detect press/release transitions
  // for logging. The published Joy state is always read fresh from
  // device_ in publish_joy_state().
  bool a_button_down_ = false;
  bool b_button_down_ = false;
  bool two_button_down_ = false;

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<sensor_msgs::msg::Joy>::SharedPtr joy_publisher_;

  // publishers for raw Wiimote messages
  rclcpp::Publisher<racerbot_wiimote_msgs::msg::WiimoteButtonsRaw>::SharedPtr
      raw_buttons_publisher_;

  void poll_wiimote();
  void publish_joy_state();
  void publish_raw_button_state();

  // Reads all node parameters into the members above (and returns the
  // ones only needed locally in the constructor, like topic name).
  struct StartupParams {
    std::string joy_topic;
    int publisher_queue_depth;
    int poll_period_ms;
  };
  StartupParams declare_parameters();
};

#endif // WIIMOTE_DRIVER_NODE_HPP_