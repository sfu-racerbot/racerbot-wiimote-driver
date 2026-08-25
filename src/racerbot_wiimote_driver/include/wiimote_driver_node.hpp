#ifndef WIIMOTE_DRIVER_NODE_HPP_
#define WIIMOTE_DRIVER_NODE_HPP_

#include "racerbot_wiimote_msgs/msg/wiimote_buttons_raw.hpp"
#include "racerbot_wiimote_msgs/msg/wiimote_led.hpp"
#include "racerbot_wiimote_msgs/msg/wiimote_raw.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joy.hpp"
#include "wiimote_device.hpp"
#include <rclcpp/publisher.hpp>
#include <rclcpp/subscription.hpp>

class WiimoteDriverNode : public rclcpp::Node {
public:
  WiimoteDriverNode();

private:
  // ROS2 Parameters
  int accelerate_button_index_ = 1;
  int brake_button_index_ = 2;
  int deadman_button_index_ = 4;
  size_t num_buttons_ = 5;
  bool disable_deadman_led_ = false;

  WiimoteDevice device_;

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<sensor_msgs::msg::Joy>::SharedPtr joy_publisher_;

  // publishers/subscribers for raw Wiimote messages
  rclcpp::Publisher<racerbot_wiimote_msgs::msg::WiimoteButtonsRaw>::SharedPtr
      raw_buttons_publisher_;
  rclcpp::Publisher<racerbot_wiimote_msgs::msg::WiimoteRaw>::SharedPtr
      raw_wiimote_publisher_;

  rclcpp::Subscription<racerbot_wiimote_msgs::msg::WiimoteLED>::SharedPtr
      raw_led_subscriber_;

  void poll_wiimote();
  void publish_joy_state();
  void publish_raw_button_state();
  void publish_raw_info();

  void
  led_callback(const racerbot_wiimote_msgs::msg::WiimoteLED::SharedPtr msg);

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