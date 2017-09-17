#ifndef NORMAL_SOCKET_HPP
#define NORMAL_SOCKET_HPP

#include "socket.hpp"

namespace sp
{

class normal_socket: public socket
{
public:
  normal_socket(boost::asio::io_service& ios);

  virtual void async_connect(const acceptor::endpoint_type& peer_endpoint, connect_handler handler);
  virtual void async_accept(acceptor& acc, acceptor::endpoint_type& remote_ep, accept_handler handler);
  virtual void async_read_some(const boost::asio::mutable_buffers_1& buffers, read_handler handler);
  virtual void async_write_some(const boost::asio::const_buffers_1& buffers, write_handler handler);
  virtual void shutdown(boost::system::error_code& ec);
  virtual void close(boost::system::error_code& ec);

private:
  boost::asio::ip::tcp::socket socket_;
};

}

#endif // NORMAL_SOCKET_HPP
