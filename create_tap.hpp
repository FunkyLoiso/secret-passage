#ifndef CREATE_TAP_HPP
#define CREATE_TAP_HPP

#include "scoped_descriptor.hpp"

namespace sp
{

// returns interface descriptor, invalid one on error
// if inout_dev is empty, os-allocated device name is written to inout_dev
shared_descriptor create_tap(std::string* inout_dev = nullptr);

}

#endif // CREATE_TAP_HPP
