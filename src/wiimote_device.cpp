#include "wiimote_device.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <sys/poll.h>
