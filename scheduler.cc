#include "include/scheduler.hh"
#include "include/reply_builder.hh"
#include "include/req_server.hh"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <string>
#include <seastar/core/metrics_registration.hh>
#include <seastar/core/metrics.hh>
#include <seastar/core/metrics_types.hh>
#include <seastar/core/metrics_api.hh>
#include <seastar/core/scheduling.hh>

using namespace seastar;
namespace pt = boost::property_tree;

std::vector<unsigned> utilization;

distributed<scheduler> sched_server;

void scheduler::start() {
    utilization.resize(smp::count, 0);
}

void scheduler::new_service(std::string service) {
}

void* scheduler::get_req_states(std::string key) {
    return req_map[key];
}

void* scheduler::get_wf_states(std::string key) {
    return wf_map[key];
}

void scheduler::set_req_states(std::string key, void* states) {
    req_map[key] = states;
}

void scheduler::set_wf_states(std::string key, void* states) {
    wf_map[key] = states;
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

/**
 * This function return the different name label values
 *  for the named metric.
 *
 *  @note: If the statistic or label doesn't exist, the test
 *  that calls this function will fail.
 *
 * @param metric_name - the metric name
 * @param label_name - the label name
 * @return a set containing all the different values
 *         of the label.
 */
static double get_utilization(void) {
    namespace smi = seastar::metrics::impl;
    auto all_metrics = smi::get_values();
    auto& values = all_metrics->values;
    const auto& all_metadata = *all_metrics->metadata;
    int i = 33;
    auto mt = all_metadata[i].metrics.begin();
    for (auto v : values[i]) {
        if (mt->id.full_name() == "reactor_utilization") {
            double tmpd = v.d();
            return tmpd;
        }
        mt++;
    }
    return 0;
}

future<> scheduler::new_req(std::unique_ptr<httpd::request> req, std::string req_id, 
		sstring service, sstring function, std::string args, output_stream<char>& out) {
    uint64_t ts = count++;
    auto key = service + req_id + "reply";
    auto new_states = new local_reply_states;
    new_states->local = true;
    auto f = new_states->res.get_future();
    req_map[key] = (void*)new_states;

    auto workflow_states = new wf_states;
    workflow_states->ts = ts;
    wf_map[req_id] = (void*)workflow_states;

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

    workflow_states->q.push(
        make_task(default_scheduling_group(), [this, req_id, service, function, args, &out] () {
            local_req_server().js_req(req_id, service, function, args, out);
        })
    );

    local_sched.new_wf(workflow_states);

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

future<> scheduler::run_func(size_t prev_cpu, std::string req_id, std::string call_id, 
		std::string prev_service, std::string service, std::string function, std::string jsargs) {
    auto cpu = engine().cpu_id();
    auto new_states = new reply_states;
    new_states->local = false;
    new_states->prev_cpuid = prev_cpu;
    auto key = service + call_id + "reply";
    req_map[key] = (void*)new_states;

    auto workflow_states = (struct wf_states*)wf_map[req_id];

    workflow_states->q.push(
        make_task(default_scheduling_group(), [this, req_id, call_id, service, function, jsargs] () {
            local_req_server().run_func(req_id, call_id, service, function, jsargs);
        })
    );

    local_sched.dispatch();
    return make_ready_future<>();
}

future<> scheduler::schedule(std::string req_id, std::string caller, std::string callee, 
		std::string prev_service, std::string service, std::string function, std::string jsargs) {
    auto cpu = engine().cpu_id();
    auto u = get_utilization();
    utilization[cpu] = u; 
    if (u < 90) {
            return run_func(cpu, req_id, callee, prev_service, service, function, jsargs);
    }
    else {
        size_t min = std::min_element(utilization.begin(), utilization.end()) - utilization.begin();
	if (min != cpu && utilization[min] < 90)
	    return sched_server.invoke_on(min , &scheduler::run_func, cpu, req_id,
                                  callee, prev_service, service, function, jsargs);
	else
            return run_func(cpu, req_id, callee, prev_service, service, function, jsargs);
    }
}

future<> scheduler::reply(std::string req_id, std::string call_id, std::string service, std::string ret) {
    auto cpu = engine().cpu_id();
    auto key = service + call_id + "reply";
    auto states = (struct reply_states*)req_map[key];

    req_map.erase(key);

    if (states->local) {
        pt::ptree pt;
        std::istringstream is(ret);
        read_json(is, pt);
 
        struct js_reply rep;
 
        rep._status = (httpd::reply::status_type)pt.get<int>("_status");
        rep._message = pt.get<std::string>("_message");

	auto s = (struct local_reply_states*)states;
        s->res.set_value(rep);

	delete_wf_states(req_id);

	return make_ready_future<>();
        //return states->out->write(std::move(reply_builder::build_direct(msg_ok, msg_ok.size())))
	//    .then([&states] () {
        //        free(states);
	//    });
    } else {
        if (states->prev_cpuid == cpu) {
	    auto workflow_states = (struct wf_states*)wf_map[req_id];

            workflow_states->q.push(
                std::move(make_task(default_scheduling_group(), [this, call_id, service, ret] () {
                    local_req_server().run_callback(call_id, service, ret);
                }))
            );

	    local_sched.dispatch();
            return make_ready_future<>();
	} else {
	    local_sched.dispatch();
            return req_server.invoke_on(states->prev_cpuid, &req_service::run_callback, call_id, service, ret);
	}
    }
}
