#include "wiimote_driver_node.hpp"
#include "racerbot_wiimote_msgs/msg/wiimote_accelerometer.hpp"
#include "racerbot_wiimote_msgs/msg/wiimote_battery.hpp"
#include "racerbot_wiimote_msgs/msg/wiimote_buttons.hpp"
#include "racerbot_wiimote_msgs/msg/wiimote_led.hpp"
#include "racerbot_wiimote_msgs/msg/wiimote_rumble.hpp"
#include "wiimote_device.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <rclcpp/logging.hpp>
#include <stdexcept>
#include <xwiimote.h>

namespace {
constexpr int kDefaultAccelerateButtonIndex = 1;
constexpr int kDefaultBrakeButtonIndex = 2;
constexpr int kDefaultDeadmanButtonIndex = 4;
constexpr int kDefaultPollPeriodMs = 5;

constexpr char wiimoteButtonsTopicName[] = "/wiimote/buttons";
constexpr char wiimoteLEDTopicName[] = "/wiimote/led";
constexpr char wiimoteBatteryTopicName[] = "/wiimote/battery";
constexpr char wiimoteAccelerometerTopicName[] = "/wiimote/accel";
constexpr char wiimoteRumbleTopicName[] = "/wiimote/rumble";

} // namespace

WiimoteDriverNode::WiimoteDriverNode()
    : Node("wiimote_driver_node"), device_(WiimoteDevice::open()) {
  RCLCPP_INFO(this->get_logger(), "Wiimote driver started...");

  declare_parameters();

  joy_publisher_ = this->create_publisher<sensor_msgs::msg::Joy>(joy_topic, 1);
  wiimote_buttons_publisher_ =
      this->create_publisher<racerbot_wiimote_msgs::msg::WiimoteButtons>(
          wiimoteButtonsTopicName, 1);
  wiimote_battery_publisher_ =
      this->create_publisher<racerbot_wiimote_msgs::msg::WiimoteBattery>(
          wiimoteBatteryTopicName, 1);
  wiimote_accelerometer_publisher_ =
      this->create_publisher<racerbot_wiimote_msgs::msg::WiimoteAccelerometer>(
          wiimoteAccelerometerTopicName, 1);

  wiimote_led_subscriber_ =
      this->create_subscription<racerbot_wiimote_msgs::msg::WiimoteLED>(
          wiimoteLEDTopicName, 1,
          [this](racerbot_wiimote_msgs::msg::WiimoteLED::SharedPtr msg) {
            this->led_callback(msg);
          });

  wiimote_rumble_subscriber_ =
      this->create_subscription<racerbot_wiimote_msgs::msg::WiimoteRumble>(
          wiimoteRumbleTopicName, 1,
          [this](racerbot_wiimote_msgs::msg::WiimoteRumble::SharedPtr msg) {
            this->rumble_callback(msg);
          });

  timer_ = this->create_wall_timer(std::chrono::milliseconds(poll_period_ms),
                                   [this]() { poll_wiimote(); });

  // Make sure first player light is enabled to show the Wiimote is connected
  device_.set_led(0, true);
}

void WiimoteDriverNode::poll_wiimote() {
  try {
    device_.update();
    publish_button_state();
    publish_battery_info();
    publish_accelerometer();

    // Have a light on to show the deadman switch is being held
    if (!disable_deadman_led_) {
      if (device_.is_button_down(XWII_KEY_TWO)) {
        device_.set_led(1, true);
      } else {
        device_.set_led(1, false);
      }
    }

    publish_joy_state();
  } catch (const std::exception &e) {
    RCLCPP_ERROR(this->get_logger(), "%s", e.what());
  }
}

void WiimoteDriverNode::publish_joy_state() {
  sensor_msgs::msg::Joy joy_msg;
  joy_msg.header.stamp = this->now();

  if (accelerometer_axis_index_ < 0) {
    RCLCPP_ERROR(get_logger(), "Invalid accelerometer_axis_index: %d",
                 accelerometer_axis_index_);
    return;
  }

  joy_msg.buttons.resize(num_buttons_, 0);
  joy_msg.axes.resize(2);

  joy_msg.buttons[accelerate_button_index_] =
      static_cast<int32_t>(device_.is_button_down(XWII_KEY_A));
  joy_msg.buttons[brake_button_index_] =
      static_cast<int32_t>(device_.is_button_down(XWII_KEY_B));
  joy_msg.buttons[deadman_button_index_] =
      static_cast<int32_t>(device_.is_button_down(XWII_KEY_TWO));

  const WiimoteAccelerometerData accel = device_.get_accelerometer_state();

  joy_msg.axes.at(accelerometer_axis_index_) = accelerometer_y_to_joy(accel.y);

  joy_publisher_->publish(joy_msg);
}

void WiimoteDriverNode::declare_parameters() {

  joy_topic = this->declare_parameter<std::string>("joy_topic", "joy");
  poll_period_ms = static_cast<int>(
      this->declare_parameter<int64_t>("poll_period_ms", kDefaultPollPeriodMs));

  accelerate_button_index_ = static_cast<int>(this->declare_parameter<int64_t>(
      "accelerate_button_index", kDefaultAccelerateButtonIndex));
  brake_button_index_ = static_cast<int>(this->declare_parameter<int64_t>(
      "brake_button_index", kDefaultBrakeButtonIndex));
  deadman_button_index_ = static_cast<int>(this->declare_parameter<int64_t>(
      "deadman_button_index", kDefaultDeadmanButtonIndex));

  this->declare_parameter<bool>("disable_deadman_led", false);
  disable_deadman_led_ = this->get_parameter("disable_deadman_led").as_bool();

  // Size the buttons array to fit whichever configured index is largest.
  num_buttons_ = static_cast<size_t>(
      std::max({accelerate_button_index_, brake_button_index_,
                deadman_button_index_}) +
      1);

  accelerometer_axis_index_ = static_cast<int>(
      this->declare_parameter<int64_t>("accelerometer_axis_index", 0));

  accelerometer_y_min_ =
      this->declare_parameter<double>("accelerometer_y_min", -100.0);

  accelerometer_y_center_ =
      this->declare_parameter<double>("accelerometer_y_center", 0.0);

  accelerometer_y_max_ =
      this->declare_parameter<double>("accelerometer_y_max", 100.0);

  invert_accelerometer_y_ =
      this->declare_parameter<bool>("invert_accelerometer_y", false);

  accelerometer_filter_alpha_ =
      this->declare_parameter<double>("accelerometer_filter_alpha", 0.2);

  accelerometer_activation_threshold_ = this->declare_parameter<double>(
      "accelerometer_activation_threshold", 0.12);

  accelerometer_release_threshold_ =
      this->declare_parameter<double>("accelerometer_release_threshold", 0.08);

  if (accelerometer_axis_index_ < 0) {
    throw std::invalid_argument(
        "accelerometer_axis_index must be non-negative");
  }

  if (!(accelerometer_y_min_ < accelerometer_y_center_ &&
        accelerometer_y_center_ < accelerometer_y_max_)) {
    throw std::invalid_argument(
        "Expected accelerometer_y_min < accelerometer_y_center "
        "< accelerometer_y_max");
  }

  if (accelerometer_filter_alpha_ <= 0.0 || accelerometer_filter_alpha_ > 1.0) {
    throw std::invalid_argument(
        "accelerometer_filter_alpha must be in the range (0, 1]");
  }

  if (accelerometer_release_threshold_ < 0.0 ||
      accelerometer_activation_threshold_ > 1.0 ||
      accelerometer_release_threshold_ >= accelerometer_activation_threshold_) {
    throw std::invalid_argument(
        "Expected 0 <= release threshold < activation threshold <= 1");
  }

  num_axes_ = static_cast<size_t>(accelerometer_axis_index_) + 1;
}

void WiimoteDriverNode::publish_button_state() {
  racerbot_wiimote_msgs::msg::WiimoteButtons msg;
  msg.header.stamp = this->now();

  msg.a = this->device_.is_button_down(XWII_KEY_A);
  msg.b = this->device_.is_button_down(XWII_KEY_B);
  msg.one = this->device_.is_button_down(XWII_KEY_ONE);
  msg.two = this->device_.is_button_down(XWII_KEY_TWO);
  msg.home = this->device_.is_button_down(XWII_KEY_HOME);
  msg.plus = this->device_.is_button_down(XWII_KEY_PLUS);
  msg.minus = this->device_.is_button_down(XWII_KEY_MINUS);
  msg.up = this->device_.is_button_down(XWII_KEY_UP);
  msg.down = this->device_.is_button_down(XWII_KEY_DOWN);
  msg.left = this->device_.is_button_down(XWII_KEY_LEFT);
  msg.right = this->device_.is_button_down(XWII_KEY_RIGHT);

  wiimote_buttons_publisher_->publish(msg);
}

void WiimoteDriverNode::led_callback(
    racerbot_wiimote_msgs::msg::WiimoteLED::SharedPtr msg) {
  device_.set_led(0, msg->led_1);
  device_.set_led(1, msg->led_2);
  device_.set_led(2, msg->led_3);
  device_.set_led(3, msg->led_4);
}

void WiimoteDriverNode::publish_battery_info() {
  racerbot_wiimote_msgs::msg::WiimoteBattery msg;
  msg.header.stamp = this->now();

  msg.battery_percentage = device_.get_battery();

  wiimote_battery_publisher_->publish(msg);
}

void WiimoteDriverNode::publish_accelerometer() {
  WiimoteAccelerometerData accel_data = device_.get_accelerometer_state();

  racerbot_wiimote_msgs::msg::WiimoteAccelerometer msg;
  msg.header.stamp = this->now();

  msg.x = accel_data.x;
  msg.y = accel_data.y;
  msg.z = accel_data.z;

  wiimote_accelerometer_publisher_->publish(msg);
}

float WiimoteDriverNode::accelerometer_y_to_joy(int raw_y) {
  const double delta = static_cast<double>(raw_y) - accelerometer_y_center_;

  // Support asymmetric calibration around the center.
  const double span = delta >= 0.0
                          ? accelerometer_y_max_ - accelerometer_y_center_
                          : accelerometer_y_center_ - accelerometer_y_min_;

  double normalized = delta / span;
  normalized = std::clamp(normalized, -1.0, 1.0);

  if (invert_accelerometer_y_) {
    normalized = -normalized;
  }

  // Exponential moving-average low-pass filter.
  if (!accelerometer_filter_initialized_) {
    filtered_accelerometer_y_ = normalized;
    accelerometer_filter_initialized_ = true;
  } else {
    filtered_accelerometer_y_ +=
        accelerometer_filter_alpha_ * (normalized - filtered_accelerometer_y_);
  }

  const double magnitude = std::abs(filtered_accelerometer_y_);

  // Schmitt-trigger-style hysteresis.
  if (accelerometer_axis_active_) {
    if (magnitude <= accelerometer_release_threshold_) {
      accelerometer_axis_active_ = false;
    }
  } else {
    if (magnitude >= accelerometer_activation_threshold_) {
      accelerometer_axis_active_ = true;
    }
  }

  if (!accelerometer_axis_active_) {
    return 0.0F;
  }

  // Remove and rescale the dead zone so the remaining range still
  // reaches 1.0.
  const double output_magnitude =
      std::clamp((magnitude - accelerometer_release_threshold_) /
                     (1.0 - accelerometer_release_threshold_),
                 0.0, 1.0);

  const double output =
      std::copysign(output_magnitude, filtered_accelerometer_y_);

  return static_cast<float>(output);
}

void WiimoteDriverNode::rumble_callback(
    const racerbot_wiimote_msgs::msg::WiimoteRumble::SharedPtr msg) {
  device_.set_rumble(msg->enabled);
}

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);

  try {
    rclcpp::spin(std::make_shared<WiimoteDriverNode>());
  } catch (const std::exception &e) {
    RCLCPP_FATAL(rclcpp::get_logger("wiimote_driver_node"), "Fatal error: %s",
                 e.what());
    rclcpp::shutdown();
    return 1;
  }

  rclcpp::shutdown();
  return 0;
}