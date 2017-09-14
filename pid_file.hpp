#ifndef PID_FILE_HPP
#define PID_FILE_HPP

#include "exception.hpp"

namespace sp
{

class pid_file
{
public:
  bool open(std::string pid_path); // false if failed
  ~pid_file();

private:
  void close();
  std::string pid_path_;
  int fd_ = 0;
};

}

#endif // PID_FILE_HPP
