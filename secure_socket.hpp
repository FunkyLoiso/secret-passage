#ifndef SECURE_SOCKET_HPP
#define SECURE_SOCKET_HPP

#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/context.hpp>
#include "socket.hpp"

namespace sp
{

class secure_socket: public socket
{
public:
  secure_socket(boost::asio::io_service& ios);

  virtual void async_connect(const acceptor::endpoint_type& peer_endpoint, connect_handler handler);
  virtual void async_accept(acceptor& acc, acceptor::endpoint_type& remote_ep, accept_handler handler);
  virtual void async_read_some(const boost::asio::mutable_buffers_1& buffers, read_handler handler);
  virtual void async_write_some(const boost::asio::const_buffers_1& buffers, write_handler handler);
  virtual void cancel();
  virtual void shutdown(boost::system::error_code& ec);
  virtual void close(boost::system::error_code& ec);

private:
  void handle_connect(const boost::system::error_code& ec, connect_handler h);
  void handle_accept(const boost::system::error_code& ec, accept_handler h);
  
  boost::asio::ssl::context ctxt_;
  boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_strm_;
};

}

#endif // SECURE_SOCKET_HPP
