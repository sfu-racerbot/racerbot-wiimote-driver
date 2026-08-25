#include "wiimote_driver_node.hpp"
#include "racerbot_wiimote_msgs/msg/wiimote_buttons_raw.hpp"

#include <chrono>
#include <rclcpp/logging.hpp>
#include <xwiimote.h>

namespace {
constexpr int kDefaultAccelerateButtonIndex = 1;
constexpr int kDefaultBrakeButtonIndex = 2;
constexpr int kDefaultDeadmanButtonIndex = 4;
constexpr int kDefaultPollPeriodMs = 5;
constexpr int kDefaultPublisherQueueDepth = 1;

constexpr char wiimoteButtonsTopicName[] = "/wiimote/buttons";

} // namespace

WiimoteDriverNode::WiimoteDriverNode()
    : Node("wiimote_driver_node"), device_(WiimoteDevice::open()) {
  RCLCPP_INFO(this->get_logger(), "Wiimote driver started...");

  StartupParams params = declare_parameters();

  joy_publisher_ = this->create_publisher<sensor_msgs::msg::Joy>(
      params.joy_topic, params.publisher_queue_depth);
  raw_buttons_publisher_ =
      this->create_publisher<racerbot_wiimote_msgs::msg::WiimoteButtonsRaw>(
          wiimoteButtonsTopicName, params.publisher_queue_depth);

  timer_ =
      this->create_wall_timer(std::chrono::milliseconds(params.poll_period_ms),
                              [this]() { poll_wiimote(); });

  // Make sure first player light is enabled to show the Wiimote is connected
  device_.set_led(0, true);
}

void WiimoteDriverNode::poll_wiimote() {
  try {
    device_.update();
    publish_raw_button_state();

    // Have a light on to show the deadman switch is being held
    if (two_button_down_) {
      device_.set_led(1, true);
    } else {
      device_.set_led(1, false);
    }

    publish_joy_state();
  } catch (const std::exception &e) {
    RCLCPP_ERROR(this->get_logger(), "%s", e.what());
  }
}

void WiimoteDriverNode::publish_joy_state() {
  sensor_msgs::msg::Joy joy_msg;
  joy_msg.header.stamp = this->now();
  joy_msg.buttons.resize(num_buttons_, 0);

  joy_msg.buttons[accelerate_button_index_] =
      static_cast<int32_t>(device_.is_button_down(XWII_KEY_A));
  joy_msg.buttons[brake_button_index_] =
      static_cast<int32_t>(device_.is_button_down(XWII_KEY_B));
  joy_msg.buttons[deadman_button_index_] =
      static_cast<int32_t>(device_.is_button_down(XWII_KEY_TWO));

  joy_publisher_->publish(joy_msg);
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

WiimoteDriverNode::StartupParams WiimoteDriverNode::declare_parameters() {
  StartupParams params;

  params.joy_topic = this->declare_parameter<std::string>("joy_topic", "joy");
  params.publisher_queue_depth =
      static_cast<int>(this->declare_parameter<int64_t>(
          "publisher_queue_depth", kDefaultPublisherQueueDepth));
  params.poll_period_ms = static_cast<int>(
      this->declare_parameter<int64_t>("poll_period_ms", kDefaultPollPeriodMs));

  accelerate_button_index_ = static_cast<int>(this->declare_parameter<int64_t>(
      "accelerate_button_index", kDefaultAccelerateButtonIndex));
  brake_button_index_ = static_cast<int>(this->declare_parameter<int64_t>(
      "brake_button_index", kDefaultBrakeButtonIndex));
  deadman_button_index_ = static_cast<int>(this->declare_parameter<int64_t>(
      "deadman_button_index", kDefaultDeadmanButtonIndex));

  // Size the buttons array to fit whichever configured index is largest.
  num_buttons_ = static_cast<size_t>(
      std::max({accelerate_button_index_, brake_button_index_,
                deadman_button_index_}) +
      1);

  return params;
}

void WiimoteDriverNode::publish_raw_button_state() {
  racerbot_wiimote_msgs::msg::WiimoteButtonsRaw msg;
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

  raw_buttons_publisher_->publish(msg);
}