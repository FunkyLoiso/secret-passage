#ifndef SCOPED_DESCRIPTOR_HPP
#define SCOPED_DESCRIPTOR_HPP

#include <boost/shared_ptr.hpp>

namespace sp
{

class scoped_descriptor {
public:
  static const int invalid = -1;

  explicit scoped_descriptor(int d = invalid)
    : d_(d) {}

  ~scoped_descriptor() {
    close();
  }

  explicit operator bool() const {
    return valid();
  }

  bool valid() const {
    return invalid != d_;
  }

  int get() const {
    return d_;
  }

  int release() {
    int ret = d_;
    d_ = invalid;
    return ret;
  }

  void reset(int d = invalid) {
    close();
    d_ = d;
  }

private:
  void close() {
    if(invalid != d_) {
      ::close(d_);
    };
    d_ = invalid;
  }
  int d_;
};

typedef boost::shared_ptr<scoped_descriptor> shared_descriptor;

}

#endif // SCOPED_DESCRIPTOR_HPP
