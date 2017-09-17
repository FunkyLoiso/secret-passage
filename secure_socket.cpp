#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include "secure_socket.hpp"
#include "logging.hpp"

namespace asio = boost::asio;

namespace sp
{

secure_socket::secure_socket(boost::asio::io_service& ios)
  : ctxt_(asio::ssl::context::sslv23)
  , ssl_strm_(ios, ctxt_)
{
  // @TODO: decide tls stuff
//  ctxt_.set_options(
//      boost::asio::ssl::context::default_workarounds
//      | boost::asio::ssl::context::no_sslv2
//      | boost::asio::ssl::context::single_dh_use);
//  ctxt_.use_certificate_chain_file("server.pem");
//  ctxt_.use_private_key_file("server.pem", boost::asio::ssl::context::pem);
//  ctxt_.use_tmp_dh_file("dh512.pem");
}

void secure_socket::async_connect(const acceptor::endpoint_type& peer_endpoint, connect_handler handler) {
  LOG_TRACE << __PRETTY_FUNCTION__;
  ssl_strm_.lowest_layer().async_connect(
    peer_endpoint,
    boost::bind(
      &secure_socket::handle_connect,
        this,
        asio::placeholders::error,
        handler
    )
  );
}

void secure_socket::async_accept(acceptor& acc, acceptor::endpoint_type& remote_ep, accept_handler handler) {
  LOG_TRACE << __PRETTY_FUNCTION__;
  acc.async_accept(
    ssl_strm_.lowest_layer(),
    remote_ep,
    boost::bind(
      &secure_socket::handle_accept,
        this,
        asio::placeholders::error,
        handler
    )
  );
}

void secure_socket::async_read_some(const boost::asio::mutable_buffers_1& buffers, read_handler handler) {
  LOG_TRACE << __PRETTY_FUNCTION__;
  ssl_strm_.async_read_some(buffers, handler);
}

void secure_socket::async_write_some(const boost::asio::const_buffers_1& buffers, write_handler handler) {
  LOG_TRACE << __PRETTY_FUNCTION__;
  ssl_strm_.async_write_some(buffers, handler);
}

void secure_socket::shutdown(boost::system::error_code& ec) {
  LOG_TRACE << __PRETTY_FUNCTION__;
  ssl_strm_.shutdown(ec);
  ssl_strm_.lowest_layer().shutdown(asio::socket_base::shutdown_both, ec);
}

void secure_socket::close(boost::system::error_code& ec) {
  LOG_TRACE << __PRETTY_FUNCTION__;
  ssl_strm_.lowest_layer().close(ec);
}

void secure_socket::handle_connect(const boost::system::error_code& ec, socket::connect_handler h) {
  LOG_TRACE << __PRETTY_FUNCTION__;
  if(ec) {
    return h(ec);
  }
  ssl_strm_.async_handshake(asio::ssl::stream_base::client, h);
}

void secure_socket::handle_accept(const boost::system::error_code& ec, accept_handler h) {
  LOG_TRACE << __PRETTY_FUNCTION__;
  if(ec) {
    return h(ec);
  }
  ssl_strm_.async_handshake(asio::ssl::stream_base::server, h);
}

}
