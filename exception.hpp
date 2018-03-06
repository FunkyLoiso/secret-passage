#ifndef EXCEPTION_HPP
#define EXCEPTION_HPP

#include <exception>
#include <string>
#include <boost/format.hpp>
// #include <boost/utility/string_view.hpp>

namespace sp {

template<typename TAG>
class exception: public std::exception {
public:
  exception(): what_("no message") {}
  exception(std::string what): what_(what) {}
//   exception(boost::string_view title, boost::string_view message)
//     : what_(title.data(), title.size())
//   {
//     what_.append(": ").append(message.data(), message.size());
//   }
  exception(const boost::format& bf)
    : what_(boost::str(bf)) {}

  virtual const char* what() const noexcept {
    return what_.c_str();
  }

private:
  std::string what_;
};

}

#endif // EXCEPTION_HPP
