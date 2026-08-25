#include "rclcpp/rclcpp.hpp"
#include "xwiimote.h"
#include <cstdlib>
#include <rclcpp/logging.hpp>
#include <sys/poll.h>

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
    }

private:
    struct xwii_iface *acceleration_device; // for accelerometer
    struct xwii_iface *core_device; // for buttons

    struct pollfd file_descriptors[2];
    
    rclcpp::TimerBase::SharedPtr timer_;
};

int main()
{
}