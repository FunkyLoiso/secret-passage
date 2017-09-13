#ifndef EXCEPTION_HPP
#define EXCEPTION_HPP

#include <exception>
#include <string>

namespace sp {

template<typename TAG>
class exception: public std::exception {
public:
  exception(): what_("no message") {}
  exception(std::string what): what_(what) {}
  exception(std::string title, std::string message)
    : what_(title + ": " + message) {}

  virtual const char* what() const noexcept {
    return what_.c_str();
  }

private:
  std::string what_;
};

}

#endif // EXCEPTION_HPP
