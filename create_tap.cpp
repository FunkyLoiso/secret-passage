#include <fcntl.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include "create_tap.hpp"
#include "logging.hpp"

namespace sp
{

namespace str {
static const char clone_dev[] = "/dev/net/tun";
}

shared_descriptor create_tap(std::__cxx11::string* inout_dev /*= nullptr*/) {
  auto ret = boost::make_shared<scoped_descriptor>();
  int fd = -1;
   /* open the tap device to clone */
  if( (fd = open(str::clone_dev, O_RDWR)) < 0 ) {
    LOG_FATAL << bf("Failed to open device '%s' for cloning: %s")
      % str::clone_dev % strerror(errno);
    return ret;
  }
  ret->reset(fd);

  ifreq ifr = {};
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;   /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */
  if (inout_dev && !inout_dev->empty()) {
   /* if a device name was specified, put it in the structure; otherwise,
    * the kernel will try to allocate the "next" device of the
    * specified type */
   strncpy(ifr.ifr_name, inout_dev->c_str(), IFNAMSIZ);
  }

  /* try to create the device */
  if(-1 == ioctl(fd, TUNSETIFF, (void *) &ifr)) {
    LOG_FATAL << bf("ioctl TUNSETIFF failed with ifr_flags = IFF_TAP, ifr_name = '%s': %s")
      % ifr.ifr_name % strerror(errno);
    ret->release();
    return ret;
  }

  if(inout_dev) {
    *inout_dev = ifr.ifr_name;
  }
  return ret;
}

}
