#include <cstring>
#include "util/error.hpp"

namespace util
{

std::string ErrnoToMessage(int errno_)
{
  char buffer[256];
  if (strerror_r(errno_, buffer, sizeof(buffer)) < 0) return "";
  return buffer;
}

SystemError::SystemError(int errno_) :
  std::runtime_error(ErrnoToMessage(errno_)),
  errno_(errno_)
{
}

} /* fs namespace */
