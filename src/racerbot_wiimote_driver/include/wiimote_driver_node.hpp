#ifndef WIIMOTE_DRIVER_NODE_HPP_
#define WIIMOTE_DRIVER_NODE_HPP_

#include "racerbot_wiimote_msgs/msg/wiimote_accelerometer.hpp"
#include "racerbot_wiimote_msgs/msg/wiimote_battery.hpp"
#include "racerbot_wiimote_msgs/msg/wiimote_buttons.hpp"
#include "racerbot_wiimote_msgs/msg/wiimote_led.hpp"
#include "racerbot_wiimote_msgs/msg/wiimote_rumble.hpp"
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
  rclcpp::Publisher<racerbot_wiimote_msgs::msg::WiimoteButtons>::SharedPtr
      wiimote_buttons_publisher_;
  rclcpp::Publisher<racerbot_wiimote_msgs::msg::WiimoteBattery>::SharedPtr
      wiimote_battery_publisher_;
  rclcpp::Publisher<racerbot_wiimote_msgs::msg::WiimoteAccelerometer>::SharedPtr
      wiimote_accelerometer_publisher_;

  rclcpp::Subscription<racerbot_wiimote_msgs::msg::WiimoteLED>::SharedPtr
      wiimote_led_subscriber_;
  rclcpp::Subscription<racerbot_wiimote_msgs::msg::WiimoteRumble>::SharedPtr
      wiimote_rumble_subscriber_;

  void declare_parameters();

  void poll_wiimote();
  void publish_joy_state();
  void publish_button_state();
  void publish_battery_info();
  void publish_accelerometer();

  void
  led_callback(const racerbot_wiimote_msgs::msg::WiimoteLED::SharedPtr msg);
  void rumble_callback(
      const racerbot_wiimote_msgs::msg::WiimoteRumble::SharedPtr msg);

  float accelerometer_y_to_joy(int raw_y);

  std::string joy_topic;
  int poll_period_ms;

  // Accelerometer to Joystick input parameters
  int accelerometer_axis_index_ = 0;

  double accelerometer_y_min_ = -100.0;
  double accelerometer_y_center_ = 0.0;
  double accelerometer_y_max_ = 100.0;

  bool invert_accelerometer_y_ = false;

  // 1.0 = no filtering, smaller values = more smoothing
  double accelerometer_filter_alpha_ = 0.2;

  // Hysteresis thresholds in normalized joystick units.
  double accelerometer_activation_threshold_ = 0.12;
  double accelerometer_release_threshold_ = 0.08;

  size_t num_axes_ = 1;

  bool accelerometer_filter_initialized_ = false;
  bool accelerometer_axis_active_ = false;
  double filtered_accelerometer_y_ = 0.0;
};

#endif // WIIMOTE_DRIVER_NODE_HPP_