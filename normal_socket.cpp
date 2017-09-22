#include "normal_socket.hpp"
#include "logging.hpp"

namespace sp
{

normal_socket::normal_socket(boost::asio::io_service& ios)
  : socket_(ios)
{}

void normal_socket::async_connect(const acceptor::endpoint_type& peer_endpoint, connect_handler handler) {
  LOG_TRACE << __PRETTY_FUNCTION__;
  socket_.async_connect(peer_endpoint, handler);
}

void normal_socket::async_accept(acceptor& acc, acceptor::endpoint_type& remote_ep, accept_handler handler) {
  LOG_TRACE << __PRETTY_FUNCTION__;
  acc.async_accept(socket_, remote_ep, handler);
}

void normal_socket::async_read_some(const boost::asio::mutable_buffers_1& buffers, read_handler handler) {
  LOG_TRACE << __PRETTY_FUNCTION__;
  socket_.async_read_some(buffers, handler);
}

void normal_socket::async_write_some(const boost::asio::const_buffers_1& buffers, write_handler handler) {
  LOG_TRACE << __PRETTY_FUNCTION__;
  socket_.async_write_some(buffers, handler);
}

void normal_socket::async_write_some(const std::vector<boost::asio::const_buffer>& buffers, socket::write_handler handler) {
  LOG_TRACE << __PRETTY_FUNCTION__;
  socket_.async_write_some(buffers, handler);
}

void normal_socket::cancel(boost::system::error_code& ec) {
  LOG_TRACE << __PRETTY_FUNCTION__;
  socket_.cancel(ec);
}

void normal_socket::shutdown(boost::system::error_code& ec) {
  LOG_TRACE << __PRETTY_FUNCTION__;
  socket_.shutdown(boost::asio::socket_base::shutdown_both, ec);
}

void normal_socket::close(boost::system::error_code& ec) {
  LOG_TRACE << __PRETTY_FUNCTION__;
  socket_.close(ec);
}

}
