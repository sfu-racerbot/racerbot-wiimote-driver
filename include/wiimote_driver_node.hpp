#ifndef WIIMOTE_DRIVER_NODE_HPP_
#define WIIMOTE_DRIVER_NODE_HPP_

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joy.hpp"
#include "xwiimote.h"

#include <sys/poll.h>

// Find the device path of the first available Wiimote.
// Caller owns the returned string and must free() it.
char *find_wiimote();

class WiimoteDriverNode : public rclcpp::Node
{
public:
    WiimoteDriverNode();
    ~WiimoteDriverNode() override;

private:
    // Button indices within the published Joy message's buttons array.
    static constexpr int kAccelerateButton = 1;
    static constexpr int kBrakeButton = 2;
    static constexpr int kDeadmanButton = 4;
    static constexpr size_t kNumButtons = 5; // large enough for the highest index above

    struct xwii_iface *core_device_ = nullptr; // buttons interface
    struct pollfd file_descriptor_{};

    bool a_button_down_ = false;
    bool b_button_down_ = false;
    bool two_button_down_ = false;

    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<sensor_msgs::msg::Joy>::SharedPtr joy_publisher_;

    void poll_wiimote();
    void publish_joy_state();
};

#endif // WIIMOTE_DRIVER_NODE_HPP_