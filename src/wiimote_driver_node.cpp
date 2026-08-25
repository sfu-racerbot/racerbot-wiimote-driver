#include "rclcpp/rclcpp.hpp"
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

        file_descriptors[0].fd = xwii_iface_get_fd(core_device);
        file_descriptors[0].events = POLLIN;

        free(path);

        auto timer_callback = 
            [this]() -> void {
                poll_wiimote();
            };

        timer_ = this->create_wall_timer(5ms, timer_callback);
    }

private:
    struct xwii_iface *acceleration_device; // for accelerometer
    struct xwii_iface *core_device; // for buttons

    struct xwii_event event;

    struct pollfd file_descriptors[1];
    
    rclcpp::TimerBase::SharedPtr timer_;

    void poll_wiimote() {
        int err = poll(file_descriptors, 1, -1);
        if (err < 0) {
            if (errno == EINTR)
                return;
            RCLCPP_ERROR(this->get_logger(), "Failed to poll file descriptors");
            return;
        }

        while ((err = xwii_iface_dispatch(core_device, &event, sizeof(event))) == 0) {
            if (event.type == XWII_EVENT_KEY) {
                if (event.v.key.code == 4 && event.v.key.state == 1) {
                    RCLCPP_INFO(this->get_logger(), "A Button Pressed\n");
                } else if (event.v.key.code == 4 && event.v.key.state == 0) {
                    RCLCPP_INFO(this->get_logger(), "A Button Released\n");
                }
            }
        }

        if (err != -EAGAIN) {
            fprintf(stderr, "\nError reading event: %d\n", err);
            return;
        }
    }
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<WiimoteDriverNode>());
    rclcpp::shutdown();
    return 0;
}