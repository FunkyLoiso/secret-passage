#ifndef HTTP_PARSER_HPP
#define HTTP_PARSER_HPP

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include "http-parser/http_parser.h"

namespace sp
{

class http_parser;

class http_parser_handler {
public:
  virtual void handle_headers_complete(const http_parser* parser) = 0;
  // true to continue
  virtual bool handle_body(const http_parser* parser, const char* data, std::size_t size) = 0;
};

class http_parser
{
public:
  http_parser(http_parser_handler* h);
  std::size_t notify(const char* data, std::size_t size); // returns number of bytes consumed
  bool headers_complete() const;
  bool body_complete() const;
  bool failed(); // true if in error state
  std::string error();  // get error string, empty if no error

  void reset();

  std::size_t code() const;
  const std::string& status() const;

  struct url_t {
    std::string full;
    std::string host;
    uint16_t port;
    std::string path;
    std::string query;
    std::string fragment;
    std::string userinfo;
  };

  const url_t& url() const;
  const std::map<std::string, std::string>& headers() const;

private:
  struct private_t;
  boost::shared_ptr<private_t> p;
};

}

#endif // HTTP_PARSER_HPP
