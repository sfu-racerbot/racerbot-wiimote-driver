#include "wiimote_driver_node.hpp"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <rclcpp/logging.hpp>

using namespace std::chrono_literals;

char *find_wiimote()
{
  struct xwii_monitor *mon;
  char *ent, *res = nullptr;

  mon = xwii_monitor_new(false, false);
  if (!mon)
  {
    return nullptr;
  }

  ent = xwii_monitor_poll(mon);
  if (ent)
  {
    res = strdup(ent);
    free(ent);
  }

  xwii_monitor_unref(mon);
  return res;
}

WiimoteDriverNode::WiimoteDriverNode() : Node("wiimote_driver_node")
{
  RCLCPP_INFO(this->get_logger(), "Wiimote driver started...");

  char *path = find_wiimote();
  if (!path)
  {
    RCLCPP_ERROR(this->get_logger(), "Failed to locate Wiimote");
    throw std::runtime_error("No Wiimote found");
  }

  int err = xwii_iface_new(&core_device_, path);
  free(path);
  if (err)
  {
    RCLCPP_ERROR(this->get_logger(),
                 "Cannot open Wiimote device (Error code: %d)", err);
    throw std::runtime_error("Failed to create Wiimote interface");
  }

  err = xwii_iface_open(core_device_, XWII_IFACE_CORE);
  if (err)
  {
    RCLCPP_ERROR(this->get_logger(),
                 "Cannot open core interface (Error code: %d)", err);
    xwii_iface_unref(core_device_);
    core_device_ = nullptr;
    throw std::runtime_error("Failed to open core interface");
  }

  file_descriptor_.fd = xwii_iface_get_fd(core_device_);
  file_descriptor_.events = POLLIN;

  joy_publisher_ = this->create_publisher<sensor_msgs::msg::Joy>("joy", 1);

  timer_ = this->create_wall_timer(5ms, [this]()
                                   { poll_wiimote(); });
}

WiimoteDriverNode::~WiimoteDriverNode()
{
  if (core_device_)
  {
    xwii_iface_close(core_device_, XWII_IFACE_CORE);
    xwii_iface_unref(core_device_);
  }
}

void WiimoteDriverNode::poll_wiimote()
{
  int ready = poll(&file_descriptor_, 1, 0);
  if (ready < 0)
  {
    if (errno == EINTR)
    {
      return;
    }
    RCLCPP_ERROR(this->get_logger(), "Failed to poll file descriptor");
    return;
  }
  if (ready == 0)
  {
    // No event available right now; nothing to dispatch this cycle.
    return;
  }

  struct xwii_event event;
  int err;
  while ((err = xwii_iface_dispatch(core_device_, &event, sizeof(event))) == 0)
  {
    if (event.type != XWII_EVENT_KEY)
    {
      continue;
    }

    const bool pressed = (event.v.key.state == 1);

    switch (event.v.key.code)
    {
    case XWII_KEY_A:
      a_button_down_ = pressed;
      RCLCPP_INFO(this->get_logger(), "A Button %s",
                  pressed ? "Pressed" : "Released");
      break;
    case XWII_KEY_B:
      b_button_down_ = pressed;
      RCLCPP_INFO(this->get_logger(), "B Button %s",
                  pressed ? "Pressed" : "Released");
      break;
    case XWII_KEY_Y:
      two_button_down_ = pressed;
      RCLCPP_INFO(this->get_logger(), "2 Button %s",
                  pressed ? "Pressed" : "Released");
      break;
    default:
      break;
    }
  }

  if (err != -EAGAIN)
  {
    RCLCPP_ERROR(this->get_logger(), "Error reading event: %d", err);
    return;
  }

  publish_joy_state();
}

void WiimoteDriverNode::publish_joy_state()
{
  sensor_msgs::msg::Joy joy_msg;
  joy_msg.header.stamp = this->now();
  joy_msg.buttons.resize(kNumButtons, 0);

  joy_msg.buttons[kAccelerateButton] = static_cast<int32_t>(a_button_down_);
  joy_msg.buttons[kBrakeButton] = static_cast<int32_t>(b_button_down_);
  joy_msg.buttons[kDeadmanButton] = static_cast<int32_t>(two_button_down_);

  joy_publisher_->publish(joy_msg);
}

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);

  try
  {
    rclcpp::spin(std::make_shared<WiimoteDriverNode>());
  }
  catch (const std::exception &e)
  {
    RCLCPP_FATAL(rclcpp::get_logger("wiimote_driver_node"),
                 "Fatal error: %s", e.what());
    rclcpp::shutdown();
    return 1;
  }

  rclcpp::shutdown();
  return 0;
}
