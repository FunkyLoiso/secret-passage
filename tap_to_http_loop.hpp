#ifndef TAP_TO_HTTP_LOOP_HPP
#define TAP_TO_HTTP_LOOP_HPP

#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include "socket.hpp"
#include "loop_stop.hpp"
#include "http_parser.hpp"

namespace sp
{

class tap_to_http_loop {
public:
  tap_to_http_loop(boost::shared_ptr<socket> socket, boost::asio::posix::stream_descriptor& tap, loop_stop_handler lsh);
  void start();

private:
  class private_t;
  boost::shared_ptr<private_t> p;
};

}

#endif // TAP_TO_HTTP_LOOP_HPP
