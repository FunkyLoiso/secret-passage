#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/function.hpp>

namespace sp
{

class socket {
public:
  typedef boost::asio::ip::tcp::acceptor acceptor;
  typedef boost::function<void(const boost::system::error_code&)> connect_handler;
  virtual void async_connect(const acceptor::endpoint_type& peer_endpoint, connect_handler handler) = 0;

  typedef boost::function<void(const boost::system::error_code&)> accept_handler;
  virtual void async_accept(acceptor& acc, acceptor::endpoint_type& remote_ep, accept_handler handler) = 0;

  typedef boost::function<void(const boost::system::error_code&, std::size_t)> read_handler;
  virtual void async_read_some(const boost::asio::mutable_buffers_1& buffers, read_handler handler) = 0;

  typedef boost::function<void(const boost::system::error_code&, std::size_t)> write_handler;
  virtual void async_write_some(const boost::asio::const_buffers_1& buffers, write_handler handler) = 0;
  virtual void async_write_some(const std::vector<boost::asio::const_buffer>& buffers, write_handler handler) = 0;

  virtual void cancel() = 0;
  virtual void shutdown(boost::system::error_code& ec) = 0;
  virtual void close(boost::system::error_code& ec) = 0;
};

}

#endif // SOCKET_HPP
