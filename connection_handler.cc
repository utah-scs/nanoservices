#include "include/connection_handler.hh"
#include "include/req_server.hh"
#include "include/scheduler.hh"
#include "include/reply_builder.hh"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <string>

using namespace seastar;
using namespace boost::uuids;
namespace pt = boost::property_tree;

/*void connection_handler::prepare_request()
{
    _request_args._command_args_count = _parser._args_count - 1;
    _request_args._command_args = std::move(_parser._args_list);
    _request_args._tmp_keys.clear();
    _request_args._tmp_key_values.clear();
    _request_args._tmp_key_scores.clear();
    _request_args._tmp_key_value_pairs.clear();
}
*/

static short hex_to_byte(char c) {
    if (c >='a' && c <= 'z') {
        return c - 'a' + 10;
    } else if (c >='A' && c <= 'Z') {
        return c - 'A' + 10;
    }
    return c - '0';
}

/**
 * Convert a hex encoded 2 bytes substring to char
 */
static char hexstr_to_char(const compat::string_view& in, size_t from) {

    return static_cast<char>(hex_to_byte(in[from]) * 16 + hex_to_byte(in[from + 1]));
}

static bool url_decode(const compat::string_view& in, sstring& out) {
    size_t pos = 0;
    sstring buff(in.length(), 0);
    for (size_t i = 0; i < in.length(); ++i) {
        if (in[i] == '%') {
            if (i + 3 <= in.size()) {
                buff[pos++] = hexstr_to_char(in, i + 1);
                i += 2;
            } else {
                return false;
            }
        } else if (in[i] == '+') {
            buff[pos++] = ' ';
        } else {
            buff[pos++] = in[i];
        }
    }
    buff.resize(pos);
    out = buff;
    return true;
}

/**
 * Add a single query parameter to the parameter list
 */
static void add_param(request& req, const compat::string_view& param) {
    size_t split = param.find('=');

    if (split >= param.length() - 1) {
        sstring key;
        if (url_decode(param.substr(0,split) , key)) {
            req.query_parameters[key] = "";
        }
    } else {
        sstring key;
        sstring value;
        if (url_decode(param.substr(0,split), key)
                && url_decode(param.substr(split + 1), value)) {
            req.query_parameters[key] = value;
        }
    }

}

/**
 * Set the query parameters in the request objects.
 * query param appear after the question mark and are separated
 * by the ampersand sign
 */
static sstring set_query_param(request& req) {
    size_t pos = req._url.find('?');
    if (pos == sstring::npos) {
        return req._url;
    }
    size_t curr = pos + 1;
    size_t end_param;
    compat::string_view url = req._url;
    while ((end_param = req._url.find('&', curr)) != sstring::npos) {
        add_param(req, url.substr(curr, end_param - curr) );
        curr = end_param + 1;
    }
    add_param(req, url.substr(curr));
    return req._url.substr(0, pos);
}

static future<std::unique_ptr<httpd::request>>
read_request_body(input_stream<char>& buf, std::unique_ptr<httpd::request> req) {
    if (!req->content_length) {
        return make_ready_future<std::unique_ptr<httpd::request>>(std::move(req));
    }
    return buf.read_exactly(req->content_length).then([req = std::move(req)] (temporary_buffer<char> body) mutable {
        req->content = seastar::to_sstring(std::move(body));
        return make_ready_future<std::unique_ptr<httpd::request>>(std::move(req));
    });
}

inline unsigned getcpu(const sstring& key) {
    return std::hash<sstring>()(key) % smp::count;
}

future<> connection_handler::handle(input_stream<char>& in, output_stream<char>& out) {
    _parser.init();

    return in.consume(_parser).then([this, &in, &out] () -> future<> {

        if (_parser.eof()) {
            printf("Parser eof\n");
            return make_ready_future<>();
        }
        std::unique_ptr<httpd::request> req = _parser.get_parsed_request();
//        if (_server._credentials) {
//            req->protocol_name = "https";
//        }

//        size_t content_length_limit = _server.get_content_length_limit();
        sstring length_header = req->get_header("Content-Length");
        req->content_length = strtol(length_header.c_str(), nullptr, 10);

//        if (req->content_length > content_length_limit) {
//            auto msg = format("Content length limit ({}) exceeded: {}", content_length_limit, req->content_length);
//            generate_error_reply_and_close(std::move(req), reply::status_type::payload_too_large, std::move(msg));
//            return make_ready_future<>();
//        }
        return read_request_body(in, std::move(req)).then([this, &out] (std::unique_ptr<httpd::request> req) {
	    auto gen = random_generator();
	    auto req_id = to_string(gen());

	    sstring url = set_query_param(*req.get());
	    size_t split = url.substr(1).find('/');
	    
	    sstring service = url.substr(1, split);
	    sstring function = url.substr(split + 2);
	    sstring auth = req->get_header("Authorization");
	    sstring host = req->get_header("Host");

	    pt::ptree root;
	    pt::ptree headers;
	    pt::ptree parameters;

	    for (auto it = req->_headers.begin(); it != req->_headers.end(); ++it) {
	        headers.put(it->first.c_str(), it->second.c_str());
	    }

	    for (auto it = req->query_parameters.begin(); it != req->query_parameters.end(); ++it) {
	        parameters.put(it->first.c_str(), it->second.c_str());
	    }

	    root.add_child("headers", headers);
	    root.add_child("parameters", parameters);
	    root.put("content", req->content.c_str());
	    std::stringstream args;
	    pt::write_json(args, root);

	    return get_local_sched()->new_req(std::move(req), req_id, service + ".js", function, args.str(), out);
        });

        std::abort();
    });
}
