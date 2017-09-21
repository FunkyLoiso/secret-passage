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

  struct header_state_t {
    enum code_t {field, value};
  };

  struct info_t {
    std::size_t code = 0;
    std::string status;
    url_t url = {};
    std::map<std::string, std::string> headers;

    std::string cur_header_field;
    header_state_t::code_t header_state = header_state_t::field;

    bool headers_complete = false;
    bool body_complete = false;
  } info;
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
  info.url.full.append(data, size);

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
        info.url.host.append(part_beg, part_len);
        break;
      case ::UF_PORT:
        info.url.port = url_parts.port;
        break;
      case ::UF_PATH:
        info.url.path.append(part_beg, part_len);
        break;
      case ::UF_QUERY:
        info.url.query.append(part_beg, part_len);
        break;
      case ::UF_FRAGMENT:
        info.url.fragment.append(part_beg, part_len);
        break;
      case ::UF_USERINFO:
        info.url.userinfo.append(part_beg, part_len);
        break;
      }
    }
  }
  return 0;
}

int http_parser::private_t::on_status(const char* data, std::size_t size) {
  info.status.append(data, size);
  return 0;
}

int http_parser::private_t::on_header_field(const char* data, std::size_t size) {
  if(header_state_t::value == info.header_state) {
    info.cur_header_field.clear();
    info.header_state = header_state_t::field;
  }
  info.cur_header_field.append(data, size);
  return 0;
}

int http_parser::private_t::on_header_value(const char* data, std::size_t size) {
  assert(!info.cur_header_field.empty());
  info.header_state = header_state_t::value;
  info.headers[info.cur_header_field].append(data, size);
  return 0;
}

int http_parser::private_t::on_headers_complete() {
  info.headers_complete = true;
  return h->handle_headers_complete(parent) ? 0 : -1;
}

int http_parser::private_t::on_body(const char* data, std::size_t size) {
  return h->handle_body(parent, data, size) ? 0 : -1;
}

int http_parser::private_t::on_message_complete() {
  info.body_complete = true;
  return 0;
}


/*\
 *  class http_parser
\*/
http_parser::http_parser(http_parser_handler* h)
  : p(new private_t())
{
  p->h = h;
  p->parent = this;
  ::http_parser_settings_init(&p->st);
#define __CB(name) p->st.name = &private_t::http_cb<private_t::cb_type::name>
#define __DATA_CB(name) p->st.name = &private_t::http_data_cb<private_t::cb_type::name>

  __DATA_CB(on_url);
  __DATA_CB(on_status);
  __DATA_CB(on_header_field);
  __DATA_CB(on_header_value);
  __CB(on_headers_complete);
  __DATA_CB(on_body);
  __CB(on_message_complete);

#undef __DATA_CB
#undef __CB

  reset();
}

std::size_t http_parser::notify(const char* data, std::size_t size) {
  return ::http_parser_execute(&p->parser, &p->st, data, size);
}

bool http_parser::headers_complete() const {
  return p->info.headers_complete;
}

bool http_parser::body_complete() const {
  return p->info.body_complete;
}

bool http_parser::failed() {
  return ::HPE_OK != p->parser.http_errno;
}

std::string http_parser::error() {
  auto err = static_cast<::http_errno>(p->parser.http_errno);
  if(::HPE_OK == err) return std::string();
  return boost::str(bf("%s: %s") % ::http_errno_name(err) % ::http_errno_description(err));
}

void http_parser::reset() {
  ::http_parser_init(&p->parser, ::HTTP_BOTH);
  p->parser.data = p.get();
  p->info = private_t::info_t();
}

std::size_t http_parser::code() const {
  return p->parser.status_code;
}

const std::string& http_parser::status() const {
  return p->info.status;
}

const http_parser::url_t& http_parser::url() const {
  return p->info.url;
}

const std::map<std::string, std::string>& http_parser::headers() const {
  return p->info.headers;
}

}
