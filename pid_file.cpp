#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <boost/format.hpp>

#include "logging.hpp"
#include "pid_file.hpp"

namespace sp
{

bool pid_file::open(std::string pid_path)
{
  fd_ = ::open(pid_path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, S_IRUSR | S_IWUSR);
  if (fd_ == -1) {
      LOG_FATAL << bf("Failed to create pid file '%s': %s") % pid_path % strerror(errno);
      fd_ = 0;
      return false;
  }
  pid_path_ = std::move(pid_path);

  // try lock file
  flock fl;
  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = 0;
  fl.l_len = 0;
  auto res = fcntl(fd_, F_SETLK, &fl);
  if (-1 == res) {
      if (errno  == EAGAIN || errno == EACCES) {
          LOG_FATAL << bf("pid file '%s' already locked. Serice already running?") % pid_path;
      }
      else {
          LOG_FATAL << bf("unable to lock pid file '%s': %s") % pid_path % strerror(errno);
      }
      return false;
  }

  if (-1 == ftruncate(fd_, 0)) {
      LOG_FATAL << bf("failed to truncate pid file '%s': %s") % pid_path % strerror(errno);
      return false;
  }

  auto pid_str = std::to_string(getpid());
  if(write(fd_, pid_str.data(), pid_str.size()) != pid_str.size()) {
      LOG_FATAL << bf("failed to write pid file '%s': %s") % pid_path % strerror(errno);
      return false;
  }

  return true;
}

pid_file::~pid_file() {
  close();
}

void pid_file::close() {
  if(!fd_) return;
  BOOST_LOG_FUNCTION();
  if(-1 == ::close(fd_)) {
    LOG_ERROR << "close failed: " << strerror(errno);
  }
  fd_ = 0;
  if(-1 == unlink(pid_path_.c_str())) {
    LOG_ERROR << bf("unlink failed for '%s': %s") % pid_path_ % strerror(errno);
  }
}

}
