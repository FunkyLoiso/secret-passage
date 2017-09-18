#include "http_parser.hpp"
#include "logging.hpp"

namespace sp
{

/*\
 *  class http_parser::private_t
\*/
struct http_parser::private_t {
  struct cb_type {
    enum code_t {
//      on_message_begin,
      on_url, // data
      on_status,  // data
      on_header_field,  //data
      on_header_value,  //data
      on_headers_complete,
      on_body,  //data
      on_message_complete,
//      on_chunk_header,
//      on_chunk_complete
    };
  };

  template<cb_type::code_t Type>
  static int http_cb(::http_parser* parser);
  template<cb_type::code_t Type>
  static int http_data_cb(::http_parser* parser, const char* data, std::size_t size);

  int on_url(const char* data, std::size_t size);
  int on_status(const char* data, std::size_t size);
  int on_header_field(const char* data, std::size_t size);
  int on_header_value(const char* data, std::size_t size);
  int on_headers_complete();
  int on_body(const char* data, std::size_t size);
  int on_message_complete();

  http_parser_handler* h;
  ::http_parser parser;
  http_parser_settings st;
  http_parser* parent;

  url_t url;
};

template<http_parser::private_t::cb_type::code_t type>
int http_parser::private_t::http_cb(::http_parser* parser) {
  auto p = reinterpret_cast<private_t*>(parser->data);
  switch(type) {
  case cb_type::on_headers_complete: return p->on_headers_complete();
  case cb_type::on_message_complete: return p->on_message_complete();
  default: assert(!"unknwn http callback type");
  }
}

template<http_parser::private_t::cb_type::code_t type>
int http_parser::private_t::http_data_cb(::http_parser* parser, const char* data, std::size_t size) {
  auto p = reinterpret_cast<private_t*>(parser->data);
  switch(type) {
  case cb_type::on_url: return p->on_url(data, size);
  case cb_type::on_status: return p->on_status(data, size);
  case cb_type::on_header_field: return p->on_header_field(data, size);
  case cb_type::on_header_value: return p->on_header_value(data, size);
  case cb_type::on_body: return p->on_body(data, size);
  default: assert(!"unknown http data callback");
  }
}


int http_parser::private_t::on_url(const char* data, std::size_t size) {
  url.full += std::string(data, size);

  //@TODO: remove debug code
  ::http_parser_url url_parts = {};
  auto err = (::http_errno)::http_parser_parse_url(data, size, false, &url_parts);
  if(err) {
    LOG_ERROR << bf("url parsing failed: %s, %s") % ::http_errno_name(err) % ::http_errno_description(err);
    return err;
  }

  for(int i = 0; i < ::UF_MAX; ++i) {
    if(url_parts.field_set & (1 << i)) {
      auto part_beg = data + url_parts.field_data[i].off;
      auto part_len = url_parts.field_data[i].len;
      switch (i) {
      case ::UF_HOST:
        url.host.assign(part_beg, part_len);
        break;
      case ::UF_PORT:
        url.port = url_parts.port;
        break;
      case ::UF_PATH:
        url.path.assign(part_beg, part_len);
        break;
      case ::UF_QUERY:
        url.query.assign(part_beg, part_len);
        break;
      case ::UF_FRAGMENT:
        url.fragment.assign(part_beg, part_len);
        break;
      case ::UF_USERINFO:
        url.userinfo.assign(part_beg, part_len);
        break;
      }
    }
  }
  return 0;
}

int http_parser::private_t::on_status(const char* data, std::size_t size)
{

}

int http_parser::private_t::on_header_field(const char* data, std::size_t size)
{

}

int http_parser::private_t::on_header_value(const char* data, std::size_t size)
{

}

int http_parser::private_t::on_headers_complete() {
  h->handle_headers_complete(parent);
  return 0;
}

int http_parser::private_t::on_body(const char* data, std::size_t size)
{

}

int http_parser::private_t::on_message_complete()
{

}


/*\
 *  class http_parser
\*/
http_parser::http_parser(http_parser_handler* h)
  : p(new private_t())
{
  p->h = h;
  p->parent = this;
  ::http_parser_init(&p->parser, HTTP_REQUEST);
  p->parser.data = p.get();
  ::http_parser_settings_init(&p->st);
#define __CB(name) p->st.name = &private_t::http_cb<private_t::cb_type::name>
#define __DATA_CB(name) p->st.name = &private_t::http_data_cb<private_t::cb_type::name>

  __DATA_CB(on_url);
//  __DATA_CB(on_status);
//  __DATA_CB(on_header_field);
//  __DATA_CB(on_header_value);
  __CB(on_headers_complete);
//  __DATA_CB(on_body);
//  __CB(on_message_complete);

#undef __DATA_CB
#undef __CB
}

std::size_t sp::http_parser::notify(const char* data, std::size_t size) {
  return ::http_parser_execute(&p->parser, &p->st, data, size);
}

bool sp::http_parser::done() {

}

bool sp::http_parser::failed() {

}

const char*sp::http_parser::error() {

}

std::size_t sp::http_parser::code() const {

}

const std::string& sp::http_parser::status() const {

}

const http_parser::url_t& http_parser::url() const {
  return p->url;
}

const std::map<std::string, std::string> sp::http_parser::headers() const {

}

}
