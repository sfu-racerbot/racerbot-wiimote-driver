#include "wiimote_driver_node.hpp"

#include <rclcpp/logging.hpp>
#include <xwiimote.h>

using namespace std::chrono_literals;

WiimoteDriverNode::WiimoteDriverNode()
    : Node("wiimote_driver_node"), device_(WiimoteDevice::open()) {
  RCLCPP_INFO(this->get_logger(), "Wiimote driver started...");

  joy_publisher_ = this->create_publisher<sensor_msgs::msg::Joy>("joy", 1);

  timer_ = this->create_wall_timer(5ms, [this]() { poll_wiimote(); });

  // Make sure first player light is enabled to show the Wiimote is connected
  device_.set_led(0, true);
}

void WiimoteDriverNode::poll_wiimote() {
  try {
    device_.update();

    log_transition("A", XWII_KEY_A, a_button_down_);
    log_transition("B", XWII_KEY_B, b_button_down_);
    log_transition("2", XWII_KEY_TWO, two_button_down_);

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

void WiimoteDriverNode::log_transition(const char *label, unsigned int key_code,
                                       bool &was_down) {
  const bool is_down = device_.is_button_down(key_code);
  if (is_down != was_down) {
    was_down = is_down;
    RCLCPP_DEBUG(this->get_logger(), "%s Button %s", label,
                 is_down ? "Pressed" : "Released");
  }
}

void WiimoteDriverNode::publish_joy_state() {
  sensor_msgs::msg::Joy joy_msg;
  joy_msg.header.stamp = this->now();
  joy_msg.buttons.resize(kNumButtons, 0);

  joy_msg.buttons[kAccelerateButton] =
      static_cast<int32_t>(device_.is_button_down(XWII_KEY_A));
  joy_msg.buttons[kBrakeButton] =
      static_cast<int32_t>(device_.is_button_down(XWII_KEY_B));
  joy_msg.buttons[kDeadmanButton] =
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
