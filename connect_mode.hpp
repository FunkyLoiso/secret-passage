#ifndef CONNECT_MODE_HPP
#define CONNECT_MODE_HPP

#include <boost/asio/io_service.hpp>
#include "settings.hpp"
#include "scoped_descriptor.hpp"
#include "exception.hpp"

namespace sp
{

class connect_mode
{
public:
  typedef sp::exception<connect_mode> exception;

  connect_mode(boost::asio::io_service& ios, const settings& st, shared_descriptor tap);
  void setup();   // throws exception

private:
  class private_t;
  boost::shared_ptr<private_t> p;
};

}

#endif // CONNECT_MODE_HPP
