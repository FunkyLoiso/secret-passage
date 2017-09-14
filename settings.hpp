#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <boost/log/trivial.hpp>
#include "exception.hpp"

namespace sp {

class settings {
public:
  typedef sp::exception<settings> exception;
  typedef sp::exception<struct help_requested_tag> help_requested;

  static void show_help();

  settings();
  void read(int argc, char* argv[]); // throws exception, help_requested
  void clear();
  std::string to_string() const;

  std::string log_path; // syslog if empty
  boost::log::trivial::severity_level log_level; // trace, debug, info, warning, error, fatal
  std::string pid_path;
};

}

#endif // SETTINGS_HPP
