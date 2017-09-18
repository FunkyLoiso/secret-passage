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
  virtual void handle_body(const http_parser* parser, const char* data, std::size_t size) = 0;
};

class http_parser
{
public:
  http_parser(http_parser_handler* h);
  std::size_t notify(const char* data, std::size_t size); // returns number of bytes consumed
  bool done();  // true if message finished
  bool failed(); // true if in error state
  const char* error();  // get error string, nullptr is no error

  void reset();

  std::size_t code() const;
  const std::string& status() const;
  const std::string& url() const;
  const std::map<std::string, std::string> headers() const;

private:
  struct private_t;
  boost::shared_ptr<private_t> p;
};

}

#endif // HTTP_PARSER_HPP
