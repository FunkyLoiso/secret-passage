#ifndef HTTP_TO_TAP_LOOP_HPP
#define HTTP_TO_TAP_LOOP_HPP

#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/function.hpp>
#include "socket.hpp"
#include "loop_stop.hpp"
#include "http_parser.hpp"

namespace sp
{

typedef boost::function<bool(const http_parser*)> headers_complete_handler;

class http_to_tap_loop
{
public:
  http_to_tap_loop(boost::shared_ptr<socket> socket, boost::asio::posix::stream_descriptor& tap, headers_complete_handler hch, loop_stop_handler lsh);
  void start();

private:
  class private_t;
  boost::shared_ptr<private_t> p;
};

}

#endif // HTTP_TO_TAP_LOOP_HPP
