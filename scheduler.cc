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
#include <chrono>

#define LONG_FUNC 500

using namespace seastar;
using namespace std::chrono;
namespace pt = boost::property_tree;

std::vector<unsigned> utilization;
std::vector<core_states> cores;

void scheduler::dispatch(void) {
    auto i = this_shard_id();
    cores[i].mu->lock();
    if (!cores[i].q.size()) {
        cores[i].busy = false;
        auto now = high_resolution_clock::now();
        uint64_t current = duration_cast<microseconds>(now.time_since_epoch()).count()/100;

        cores[i].busy_till = current;
    }
    cores[i].mu->unlock();

    if (!cores[i].q.size()) {
	return;
    }

    engine().add_task(cores[i].q.front());
    cores[i].q.pop();
};

distributed<scheduler> sched_server;

void scheduler::start() {
    if (this_shard_id() >= HW_Q_COUNT)
        big_core = true;
    if (this_shard_id() == 0) {
        utilization.resize(smp::count, 0);
        cores.resize(smp::count, core_states());
    }
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
    for (int i = 0; i < all_metadata.size(); i++) {
        auto mt = all_metadata[i].metrics.begin();
        for (auto v : values[i]) {
            if (mt->id.full_name() == "reactor_utilization") {
                double tmpd = v.d();
                return tmpd;
            }
            mt++;
        }
    }
    return 0;
}

future<> scheduler::new_req(std::unique_ptr<httpd::request> req, std::string req_id, 
		sstring service, sstring function, std::string args, output_stream<char>& out) {
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

    auto cpu = engine().cpu_id();
    schedule(cpu, req_id, req_id, service, function, args);

    return f.then([this, &out, resp = std::move(resp), req_id] (auto&& res) {
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
        }).then([this, &out, req_id] {
            out.flush();
        });
    });
}

future<> scheduler::run_func(size_t prev_cpu, std::string req_id, std::string call_id, 
		std::string service, std::string function, std::string jsargs) {
    auto cpu = engine().cpu_id();
    auto u = get_utilization();
    utilization[cpu] = u; 

    cout << utilization << endl;

    auto key = service + call_id + "reply";

    if (req_map.find(key) == req_map.end()) {
        auto new_states = new reply_states;
        new_states->local = false;
        new_states->prev_cpuid = prev_cpu;
        req_map[key] = (void*)new_states;
    }

    auto func = service + function;

    if (func_map.find(func) == func_map.end()) {
        auto function_states = new func_states;
        auto now = high_resolution_clock::now();
        function_states->start_time = duration_cast<microseconds>(now.time_since_epoch()).count()/100;
        function_states->exec_time = -1;
        func_map[func] = (void*)function_states;
    }

    func_name_map[call_id] = func;

    auto function_states = (struct func_states*)func_map[func];

    cores[cpu].q.push(
    //engine().add_task(
        make_task(default_scheduling_group(), [this, req_id, call_id, service, function, jsargs] () {
            local_req_server().run_func(req_id, call_id, service, function, jsargs);
        })
    );

    dispatch();
    return make_ready_future<>();
}

size_t get_core(int64_t exec_time) {
    if (smp::count <= HW_Q_COUNT)
        return this_shard_id();

    uint64_t min = ULLONG_MAX;
    int min_index = HW_Q_COUNT;
    auto now = high_resolution_clock::now();
    uint64_t current = duration_cast<microseconds>(now.time_since_epoch()).count()/100;
 
    for (int i = HW_Q_COUNT; i < smp::count; i++) {
        cores[i].mu->lock();
        if (cores[i].busy_till < min) {
            min = cores[i].busy_till;
            min_index = i;
        }
        cores[i].mu->unlock();
    }
 
    if (exec_time < LONG_FUNC && exec_time != -1) {
        for (int i = 0; i < smp::count; i++) {
            cores[i].mu->lock();
           if (cores[i].busy_till < min) {
                min = cores[i].busy_till;
                min_index = i;
            }
            cores[i].mu->unlock();
        }
    }

    cores[min_index].mu->lock();
    cores[min_index].busy = true;
    cores[min_index].busy_till = std::max(cores[min_index].busy_till, current) + exec_time;
    cores[min_index].mu->unlock();

    return min_index;
}

future<> scheduler::schedule(size_t prev_cpu, std::string req_id, std::string call_id, 
		std::string service, std::string function, std::string jsargs) {
    auto cpu = engine().cpu_id();
    auto key = service + call_id + "reply";

    if (req_map.find(key) == req_map.end()) {
        auto new_states = new reply_states;
        new_states->local = false;
        new_states->prev_cpuid = prev_cpu;
        req_map[key] = (void*)new_states;
    }

    auto func = service + function;

    if (func_map.find(func) == func_map.end()) {
        auto function_states = new func_states;
        auto now = high_resolution_clock::now();
        function_states->start_time = duration_cast<microseconds>(now.time_since_epoch()).count()/100;
        function_states->exec_time = -1;
        func_map[func] = (void*)function_states;
    }

    auto function_states = (struct func_states*)func_map[func];

    func_name_map[call_id] = func;

    auto core = get_core(function_states->exec_time);
    if (core != this_shard_id())
        return sched_server.invoke_on(core, &scheduler::run_func, prev_cpu, req_id,
                                      call_id, service, function, jsargs);
    else
        return run_func(prev_cpu, req_id, call_id, service, function, jsargs);
}

future<> scheduler::reply(std::string req_id, std::string call_id, std::string service, std::string ret) {
    auto cpu = engine().cpu_id();
    auto key = service + call_id + "reply";
    auto states = (struct reply_states*)req_map[key];

    req_map.erase(key);

    auto func = func_name_map[call_id];
    func_name_map.erase(call_id);

    auto function_states = (struct func_states*)func_map[func];

    if (function_states->exec_time == -1) {
        auto now = high_resolution_clock::now();
        function_states->exec_time = duration_cast<microseconds>(now.time_since_epoch()).count()/100 - function_states->start_time;
    }

    if (states->local) {
        pt::ptree pt;
        std::istringstream is(ret);
        read_json(is, pt);
 
        struct js_reply rep;
 
        rep._status = (httpd::reply::status_type)pt.get<int>("_status");
        rep._message = pt.get<std::string>("_message");

	auto s = (struct local_reply_states*)states;
        s->res.set_value(rep);

	dispatch();
	return make_ready_future<>();
    } else {
        if (states->prev_cpuid == cpu) {
	    engine().add_task(
                std::move(make_task(default_scheduling_group(), [this, call_id, service, ret] () {
                    local_req_server().run_callback(call_id, service, ret);
                }))
            );

	    dispatch();
            return make_ready_future<>();
	} else {
            sched_server.invoke_on(states->prev_cpuid, &scheduler::reply, req_id, call_id, service, ret);
	    dispatch();
            return make_ready_future<>();
	}
    }
}
