#ifndef LISTEN_MODE_HPP
#define LISTEN_MODE_HPP

#include <boost/asio/io_service.hpp>
#include <boost/shared_ptr.hpp>
#include "settings.hpp"
#include "scoped_descriptor.hpp"
#include "exception.hpp"

namespace sp
{

class listen_mode
{
public:
  typedef exception<listen_mode> exception;

  listen_mode(boost::asio::io_service& ios, const settings& st, shared_descriptor tap);
  void setup();   // throws exception

private:
  class private_t;
  boost::shared_ptr<private_t> p;
};

}

#endif // LISTEN_MODE_HPP
