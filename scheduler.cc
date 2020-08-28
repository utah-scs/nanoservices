#include "include/scheduler.hh"
#include "include/reply_builder.hh"
#include "include/req_server.hh"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <string>

using namespace seastar;
namespace pt = boost::property_tree;

distributed<scheduler> sched_server;

void scheduler::start() {
}

void scheduler::new_service(std::string service) {
}

void* scheduler::get_req_states(std::string key) {
    return req_map[key];
}

void scheduler::set_req_states(std::string key, void* states) {
    req_map[key] = states;
}

// Write the current date in the specific "preferred format" defined in
// RFC 7231, Section 7.1.1.1, a.k.a. IMF (Internet Message Format) fixdate.
// For example: Sun, 06 Nov 1994 08:49:37 GMT
static sstring http_date() {
    auto t = ::time(nullptr);
    struct tm tm;
    gmtime_r(&t, &tm);
    // Using strftime() would have been easier, but unfortunately relies on
    // the current locale, and we need the month and day names in English.
    static const char* days[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    static const char* months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    return seastar::format("{}, {:02d} {} {} {:02d}:{:02d}:{:02d} GMT",
        days[tm.tm_wday], tm.tm_mday, months[tm.tm_mon], 1900 + tm.tm_year,
        tm.tm_hour, tm.tm_min, tm.tm_sec);
}

future<> scheduler::new_req(std::unique_ptr<request> req, std::string req_id, sstring service, sstring function, std::string args, output_stream<char>& out) {
    auto key = service + req_id + "reply";
    auto new_states = new local_reply_states;
    new_states->local = true;
    auto f = new_states->res.get_future();
    req_map[key] = (void*)new_states;

    auto resp = std::make_shared<httpd::reply>();
    bool conn_keep_alive = false;
    bool conn_close = false;
    auto it = req->_headers.find("Connection");
    if (it != req->_headers.end()) {
        if (it->second == "Keep-Alive") {
            conn_keep_alive = true;
        } else if (it->second == "Close") {
            conn_close = true;
        }
    }
    bool should_close;
    // TODO: Handle HTTP/2.0 when it releases
    resp->set_version(req->_version);

    if (req->_version == "1.0") {
        if (conn_keep_alive) {
            resp->_headers["Connection"] = "Keep-Alive";
        }
        should_close = !conn_keep_alive;
    } else if (req->_version == "1.1") {
        should_close = conn_close;
    } else {
        // HTTP/0.9 goes here
        should_close = true;
    }
    sstring version = req->_version;
    resp->set_version(version).done();
    resp->_headers["Server"] = "Seastar httpd";
    resp->_headers["Date"] = http_date();

    local_req_server().js_req(req_id, service, function, args, out);
    return f.then([&out, resp = std::move(resp)] (auto&& res) {
	resp->set_status(res._status, res._message);
	resp->done();
	resp->_response_line = resp->response_line();
        resp->_headers["Content-Length"] = to_sstring(
            resp->_content.size());
        return out.write(resp->_response_line.data(),
                resp->_response_line.size()).then([&out, resp = std::move(resp)] {
            return do_for_each(resp->_headers, [&out](auto& h) {
                return out.write(h.first + ": " + h.second + "\r\n");
            });
        }).then([&out] {
            return out.write("\r\n", 2);
        }).then([&out, resp = std::move(resp)] {
            return out.write(resp->_content.data(),
                resp->_content.size());
        }).then([&out] {
            return out.flush();
        });
    });
}

future<> scheduler::run_func(size_t cpuid, std::string req_id, std::string prev_service, 
		             std::string service, std::string function, std::string jsargs) {
    auto new_states = new reply_states;
    new_states->local = false;
    new_states->prev_cpuid = cpuid;
    auto key = service + req_id + "reply";
    req_map[key] = (void*)new_states;

    return local_req_server().run_func(req_id, service, function, jsargs);
}

future<> scheduler::schedule(std::string req_id, std::string prev_service, std::string service, 
		             std::string function, std::string jsargs) {
    return run_func(engine().cpu_id(), req_id, prev_service, service, function, jsargs);
//    return sched_server.invoke_on(engine().cpu_id() + 1 , &scheduler::run_func, engine().cpu_id(), req_id, 
//		                  prev_service, service, function, jsargs);
}

future<> scheduler::reply(std::string req_id, std::string service, std::string ret) {
    pt::ptree pt;
    std::istringstream is(ret);
    read_json(is, pt);

    struct js_reply rep;

    rep._status = (httpd::reply::status_type)pt.get<int>("_status");
    rep._message = pt.get<std::string>("_message");

    auto key = service + req_id + "reply";
    auto states = (struct reply_states*)req_map[key];

    req_map.erase(key);

    if (states->local) {
	auto s = (struct local_reply_states*)states;
        s->res.set_value(rep);
	return make_ready_future<>();
        //return states->out->write(std::move(reply_builder::build_direct(msg_ok, msg_ok.size())))
	//    .then([&states] () {
        //        free(states);
	//    });
    } else {
        return req_server.invoke_on(states->prev_cpuid, &req_service::run_callback, req_id, service, ret);
    }
}
