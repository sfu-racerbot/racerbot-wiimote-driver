#include "wiimote_device.hpp"

#include <cerrno>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <sys/poll.h>
#include <xwiimote.h>

namespace {
// Local RAII helper so struct xwii_monitor* is always released, even if
// find_device_path() returns early. Kept private to this translation unit
// since nothing outside needs to touch a monitor directly.
struct MonitorDeleter {
  void operator()(struct xwii_monitor *mon) const noexcept {
    if (mon) {
      xwii_monitor_unref(mon);
    }
  }
};
using MonitorPtr = std::unique_ptr<struct xwii_monitor, MonitorDeleter>;
} // namespace

std::optional<std::string> WiimoteDevice::find_device_path() {
  MonitorPtr mon(xwii_monitor_new(false, false));
  if (!mon) {
    return std::nullopt;
  }

  char *ent = xwii_monitor_poll(mon.get());
  if (!ent) {
    return std::nullopt;
  }

  std::string path(ent);
  free(ent);
  return path;
}

WiimoteDevice WiimoteDevice::open() {
  auto path = find_device_path();
  if (!path) {
    throw std::runtime_error("No Wiimote found");
  }

  struct xwii_iface *core_iface = nullptr;
  int err = xwii_iface_new(&core_iface, path->c_str());
  if (err) {
    throw std::runtime_error(
        "Failed to create Wiimote interface (error code: " +
        std::to_string(err) + ")");
  }

  err = xwii_iface_open(core_iface, XWII_IFACE_CORE | XWII_IFACE_WRITABLE);
  if (err) {
    xwii_iface_unref(core_iface);
    throw std::runtime_error("Failed to open core interface (error code: " +
                             std::to_string(err) + ")");
  }

  struct xwii_iface *accelerometer_iface = nullptr;
  err = xwii_iface_new(&accelerometer_iface, path->c_str());
  if (err) {
    throw std::runtime_error(
        "Failed to create Wiimote aaccelerometer interface (error code: " +
        std::to_string(err) + ")");
  }

  err = xwii_iface_open(accelerometer_iface, XWII_IFACE_ACCEL);
  if (err) {
    xwii_iface_unref(accelerometer_iface);
    throw std::runtime_error(
        "Failed to open accelerometer interface (error code: " +
        std::to_string(err) + ")");
  }

  return WiimoteDevice(core_iface, accelerometer_iface);
}

WiimoteDevice::WiimoteDevice(struct xwii_iface *core_iface,
                             struct xwii_iface *accelerometer_iface)
    : core_iface_(core_iface), accelerometer_iface_(accelerometer_iface),
      core_fd_(xwii_iface_get_fd(core_iface)),
      accelerometer_fd_(xwii_iface_get_fd(accelerometer_iface)) {
  pfds_[0].fd = core_fd_;
  pfds_[0].events = POLLIN;

  pfds_[1].fd = accelerometer_fd_;
  pfds_[1].events = POLLIN;
}

WiimoteDevice::WiimoteDevice(WiimoteDevice &&other) noexcept
    : core_iface_(other.core_iface_),
      accelerometer_iface_(other.accelerometer_iface_),
      core_fd_(other.core_fd_), accelerometer_fd_(other.accelerometer_fd_),
      button_state_(std::move(other.button_state_)) {

  pfds_[0] = other.pfds_[0];
  pfds_[1] = other.pfds_[1];

  other.release();
}

WiimoteDevice &WiimoteDevice::operator=(WiimoteDevice &&other) noexcept {
  if (this != &other) {
    // Free whatever this instance currently owns before taking ownership
    // of other's resource.
    if (core_iface_) {
      xwii_iface_close(core_iface_, XWII_IFACE_CORE);
      xwii_iface_unref(core_iface_);
    }
    core_iface_ = other.core_iface_;
    accelerometer_iface_ = other.accelerometer_iface_;
    core_fd_ = other.core_fd_;
    accelerometer_fd_ = other.accelerometer_fd_;
    button_state_ = std::move(other.button_state_);

    pfds_[0] = other.pfds_[0];
    pfds_[1] = other.pfds_[1];

    other.release();
  }
  return *this;
}

WiimoteDevice::~WiimoteDevice() {
  if (core_iface_) {
    xwii_iface_close(core_iface_, XWII_IFACE_CORE | XWII_IFACE_WRITABLE);
    xwii_iface_unref(core_iface_);
  }
  if (accelerometer_iface_) {
    xwii_iface_close(accelerometer_iface_, XWII_IFACE_ACCEL);
    xwii_iface_unref(accelerometer_iface_);
  }
}

void WiimoteDevice::release() noexcept {
  core_iface_ = nullptr;
  accelerometer_iface_ = nullptr;
  core_fd_ = -1;
  accelerometer_fd_ = -1;
  pfds_[0] = {};
  pfds_[1] = {};
}

bool WiimoteDevice::has_events() {
  // A zero timeout means "check right now, don't block".
  int ready = poll(pfds_, std::size(pfds_), 0);
  if (ready < 0) {
    if (errno == EINTR) {
      return false;
    }
    throw std::runtime_error("Failed to poll Wiimote file descriptor");
  }
  return ready > 0;
}

void WiimoteDevice::drain_events(xwii_iface *iface) {
  while (true) {
    xwii_event event{};

    const int err = xwii_iface_dispatch(iface, &event, sizeof(event));

    if (err == -EAGAIN) {
      return;
    }

    if (err != 0) {
      throw std::runtime_error("Error reading Wiimote event: " +
                               std::to_string(err));
    }

    apply_event(event);
  }
}

void WiimoteDevice::apply_event(const xwii_event &event) {
  switch (event.type) {
  case XWII_EVENT_KEY:
    button_state_[event.v.key.code] = (event.v.key.state == 1);
    break;
  case XWII_EVENT_ACCEL:
    accelerometer_data_.x = event.v.abs[0].x;
    accelerometer_data_.y = event.v.abs[0].y;
    accelerometer_data_.z = event.v.abs[0].z;
  default:
    break;
  }
}

void WiimoteDevice::update() {
  const int ready = poll(pfds_, std::size(pfds_), 0);

  if (ready < 0) {
    if (errno == EINTR) {
      return;
    }
    throw std::runtime_error("Failed to poll Wiimote file descriptors");
  }

  if (ready == 0) {
    return;
  }

  if (pfds_[0].revents & POLLIN) {
    drain_events(core_iface_);
  }

  if (pfds_[1].revents & POLLIN) {
    drain_events(accelerometer_iface_);
  }
}

bool WiimoteDevice::is_button_down(unsigned int key_code) const {
  auto it = button_state_.find(key_code);
  return it != button_state_.end() && it->second;
}

void WiimoteDevice::set_led(unsigned int led_number, bool on) {
  if (led_number >= 4)
    throw std::range_error(
        "Invalid LED number. LED number must be between 0-3");

  led_status_[led_number] = on;
  int err = xwii_iface_set_led(core_iface_, XWII_LED(led_number + 1), on);
  if (err) {
    throw std::runtime_error("Failed to set LED " + std::to_string(led_number) +
                             " (Error Code: " + std::to_string(err) + ")");
  }
}

uint8_t WiimoteDevice::get_battery() const {
  uint8_t battery_percent;
  int err = xwii_iface_get_battery(core_iface_, &battery_percent);
  if (err) {
    throw std::runtime_error(
        "Failed to read battery (Error Code: " + std::to_string(err) + ")");
  }

  return battery_percent;
}

WiimoteAccelerometerData WiimoteDevice::get_accelerometer_state() const {
  return accelerometer_data_;
}