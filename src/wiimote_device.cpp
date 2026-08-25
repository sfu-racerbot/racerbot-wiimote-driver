#include "wiimote_device.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
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

  struct xwii_iface *iface = nullptr;
  int err = xwii_iface_new(&iface, path->c_str());
  if (err) {
    throw std::runtime_error(
        "Failed to create Wiimote interface (error code: " +
        std::to_string(err) + ")");
  }

  err = xwii_iface_open(iface, XWII_IFACE_CORE | XWII_IFACE_WRITABLE);
  if (err) {
    xwii_iface_unref(iface);
    throw std::runtime_error("Failed to open core interface (error code: " +
                             std::to_string(err) + ")");
  }

  return WiimoteDevice(iface);
}

WiimoteDevice::WiimoteDevice(struct xwii_iface *iface)
    : iface_(iface), fd_(xwii_iface_get_fd(iface)) {}

WiimoteDevice::WiimoteDevice(WiimoteDevice &&other) noexcept
    : iface_(other.iface_), fd_(other.fd_),
      button_state_(std::move(other.button_state_)) {
  other.release();
}

WiimoteDevice &WiimoteDevice::operator=(WiimoteDevice &&other) noexcept {
  if (this != &other) {
    // Free whatever this instance currently owns before taking ownership
    // of other's resource.
    if (iface_) {
      xwii_iface_close(iface_, XWII_IFACE_CORE);
      xwii_iface_unref(iface_);
    }
    iface_ = other.iface_;
    fd_ = other.fd_;
    button_state_ = std::move(other.button_state_);
    other.release();
  }
  return *this;
}

WiimoteDevice::~WiimoteDevice() {
  if (iface_) {
    xwii_iface_close(iface_, XWII_IFACE_CORE);
    xwii_iface_unref(iface_);
  }
}

void WiimoteDevice::release() noexcept {
  iface_ = nullptr;
  fd_ = -1;
}

bool WiimoteDevice::has_events() const {
  struct pollfd pfd{};
  pfd.fd = fd_;
  pfd.events = POLLIN;

  // A zero timeout means "check right now, don't block".
  int ready = poll(&pfd, 1, 0);
  if (ready < 0) {
    if (errno == EINTR) {
      return false;
    }
    throw std::runtime_error("Failed to poll Wiimote file descriptor");
  }
  return ready > 0;
}

std::optional<xwii_event> WiimoteDevice::next_event() {
  struct xwii_event event{};
  int err = xwii_iface_dispatch(iface_, &event, sizeof(event));
  if (err == 0) {
    return event;
  }
  if (err == -EAGAIN) {
    return std::nullopt;
  }
  throw std::runtime_error("Error reading Wiimote event: " +
                           std::to_string(err));
}

void WiimoteDevice::apply_event(const xwii_event &event) {
  if (event.type != XWII_EVENT_KEY) {
    return;
  }
  button_state_[event.v.key.code] = (event.v.key.state == 1);
}

void WiimoteDevice::update() {
  if (!has_events()) {
    // Nothing pending; nothing to do.
    return;
  }

  while (auto event = next_event()) {
    apply_event(*event);
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
  int err = xwii_iface_set_led(iface_, XWII_LED(led_number + 1), on);
  if (err) {
    throw std::runtime_error("Failed to set LED " + std::to_string(led_number) +
                             " (Error Code: " + std::to_string(err) + ")");
  }
}