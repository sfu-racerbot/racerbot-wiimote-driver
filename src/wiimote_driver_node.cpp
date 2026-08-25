#include "rclcpp/rclcpp.hpp"
#include <sensor_msgs/msg/joy.hpp>

#include "sensor_msgs/msg/joy.hpp"
#include "xwiimote.h"
#include <cstdlib>
#include <rclcpp/logging.hpp>
#include <sys/poll.h>
#include <chrono>

using namespace std::chrono_literals;

// function to find the device path of the Wiimote
char *find_wiimote() {
  struct xwii_monitor *mon;
  char *ent, *res = NULL;

  // Create a monitor to scan for xwiimote devices
  mon = xwii_monitor_new(false, false);
  if (!mon) {
    return NULL;
  }

  ent = xwii_monitor_poll(mon);
  if (ent) {
    res = strdup(ent);
    free(ent);
  }

  xwii_monitor_unref(mon);
  return res;
}

class WiimoteDriverNode : public rclcpp::Node
{
public:
    WiimoteDriverNode() : Node("wiimote_driver_node")
    {
        RCLCPP_INFO(this->get_logger(), "Wiimote driver started...");

        char* path = find_wiimote();
        if (!path) {
            RCLCPP_ERROR(this->get_logger(), "Failed to locate Wiimote");
            std::exit(1);
        }

        int err = xwii_iface_new(&core_device, path);
        if (err) {
            RCLCPP_ERROR(this->get_logger(), "Cannot open Wiimote device (Error code: %d)\n", err);
            std::exit(1);
        }

        err = xwii_iface_open(core_device, XWII_IFACE_CORE);
        if (err) {
            RCLCPP_ERROR(this->get_logger(), "Cannot open core interface (Error code: %d)", err);
            std::exit(1);
        }

        file_descriptors[0].fd = xwii_iface_get_fd(core_device);
        file_descriptors[0].events = POLLIN;

        free(path);

        auto timer_callback = 
            [this]() -> void {
                poll_wiimote();
            };

        timer_ = this->create_wall_timer(5ms, timer_callback);
        joy_publisher_ = this->create_publisher<sensor_msgs::msg::Joy>("joy", 1);
    }

private:
    const int _deadman_button = 4;
    const int _accelerate_button = 1;
    const int _brake_button = 2;

    struct xwii_iface *acceleration_device; // for accelerometer
    struct xwii_iface *core_device; // for buttons

    struct xwii_event event;

    struct pollfd file_descriptors[1];

    bool a_button_down = false;
    bool b_button_down = false;
    bool two_button_down = false;
    
    rclcpp::TimerBase::SharedPtr timer_;

    rclcpp::Publisher<sensor_msgs::msg::Joy>::SharedPtr joy_publisher_;

    void poll_wiimote() {
        int err = poll(file_descriptors, 1, 0);
        if (err < 0) {
            if (errno == EINTR)
                return;
            RCLCPP_ERROR(this->get_logger(), "Failed to poll file descriptors");
            return;
        }

        while ((err = xwii_iface_dispatch(core_device, &event, sizeof(event))) == 0) {
            if (event.type == XWII_EVENT_KEY) {
                if (event.v.key.code == XWII_KEY_A && event.v.key.state == 1) {
                    RCLCPP_INFO(this->get_logger(), "A Button Pressed\n");
                    a_button_down = true;
                } else if (event.v.key.code == XWII_KEY_A && event.v.key.state == 0) {
                    RCLCPP_INFO(this->get_logger(), "A Button Released\n");
                    a_button_down = false;
                }

                if (event.v.key.code == XWII_KEY_B && event.v.key.state == 1) {
                    RCLCPP_INFO(this->get_logger(), "B Button Pressed\n");
                    b_button_down = true;
                } else if (event.v.key.code == XWII_KEY_B && event.v.key.state == 0) {
                    RCLCPP_INFO(this->get_logger(), "B Button Released\n");
                    b_button_down = false;
                }

                if (event.v.key.code == XWII_KEY_2 && event.v.key.state == 1) {
                    RCLCPP_INFO(this->get_logger(), "2 Button Pressed\n");
                    two_button_down = true;
                } else if (event.v.key.code == XWII_KEY_2 && event.v.key.state == 0) {
                    RCLCPP_INFO(this->get_logger(), "2 Button Released\n");
                    two_button_down = false;
                }
            }
        }

        if (err != -EAGAIN) {
            fprintf(stderr, "\nError reading event: %d\n", err);
            return;
        }

        sensor_msgs::msg::Joy joy_msg;
        joy_msg.buttons[_accelerate_button] = static_cast<int>(a_button_down);
        joy_msg.buttons[_brake_button] = static_cast<int>(b_button_down);
        joy_msg.buttons[_deadman_button] = static_cast<int>(two_button_down);

        joy_publisher_->publish(joy_msg);
    }
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<WiimoteDriverNode>());
    rclcpp::shutdown();
    return 0;
}