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

#define LONG_FUNC 50*2400000
//#define GATE 1000*2400
#define GATE 0
//#define BOOL_SCHED

using namespace seastar;
namespace pt = boost::property_tree;

std::vector<unsigned> utilization;
std::vector<core_states> cores;
std::unordered_map<std::string, void*> func_map;
std::unordered_map<std::string, void*> wf_map;

void scheduler::dispatch(void) {
    auto i = this_shard_id();
//    cores[i].mu->lock();
/*    if (!cores[i].q.size()) {
        cores[i].busy->store(false);
        uint64_t current = rdtsc();

        cores[i].busy_till->store(current);
    }*/
//    cores[i].mu->unlock();

    if (!cores[i].q.size()) {
	return;
    }


/*    engine().add_task(cores[i].q.front());
    cores[i].q.pop();*/

    cores[0].mu->lock();
    if ((!curr_wf.size()) &&
        (cores[i].task_map.find(cores[i].q.front()) != cores[i].task_map.end())) {
        engine().add_task(cores[i].task_map[cores[i].q.front()]);
	cores[i].task_map.erase(cores[i].q.front());
	cores[i].q.pop_front();
    } 
    cores[0].mu->unlock();
};

distributed<scheduler> sched_server;

int pp = 0;
uint64_t pptotal = 0;
uint64_t tsp;
void scheduler::start() {
    if (this_shard_id() >= HW_Q_COUNT)
        big_core = true;
    if (this_shard_id() == 0) {
        utilization.resize(smp::count, 0);
        cores.resize(smp::count, core_states());
	for (int i = 0; i < smp::count; i++)
	    cores[i].init();
    }
//    if (this_shard_id() ==8)
//        ping();
}

future<> scheduler::ping() {
    tsp = rdtsc();
    return sched_server.invoke_on(9, &scheduler::pong);
}

future<> scheduler::pong() {
    sched_server.invoke_on(8, &scheduler::test_reply);
    return make_ready_future<>();
}

future<> scheduler::test_reply() {
    pp++;
    pptotal += (rdtsc() - tsp);
    cout << "ping pong" << pptotal/pp << endl;
    //cout << "ping pong "<< rdtsc() - tsp << endl;
    ping();
    return make_ready_future<>();
}

void scheduler::new_service(std::string service) {
}

void* scheduler::get_req_states(std::string key) {
    return req_map[key];
}

void scheduler::set_req_states(std::string key, void* states) {
    req_map[key] = states;
}

void scheduler::del_req_states(std::string key) {
    delete req_map[key];
    req_map.erase(key);
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
        func_map[func] = (void*)function_states;
    }

    if (req_id == call_id)
        req_wf_map[req_id] = wf_map[func];

    auto workflow_states = (struct wf_states*)req_wf_map[req_id];
    auto function_states = (struct func_states*)func_map[func];

    if (req_id == call_id)
        wf_starts[req_id] = rdtsc();

    cores[0].mu->lock();
    if (workflow_states->fuse) {
        curr_wf.insert(req_id);
        engine().add_task(
        make_task(default_scheduling_group(), [this, req_id, call_id, service, function,
		                               jsargs, function_states] () {
            local_req_server().run_func(req_id, call_id, service, function, jsargs,
			                function_states);
        }));
    } else {
        curr_req.insert(call_id);
        cores[cpu].task_map[call_id] = 
        //cores[cpu].q.push(
        //engine().add_task(
            make_task(default_scheduling_group(), [this, req_id, call_id, service, function,
            	                               jsargs, function_states] () {
                local_req_server().run_func(req_id, call_id, service, function, jsargs,
            		                function_states);
            })
        ;
    //);
    }
    cores[0].mu->unlock();

    dispatch();
    return make_ready_future<>();
}

size_t get_core(uint64_t exec_time, sstring req_id, sstring call_id, sstring service) {
    auto cpu = this_shard_id();
    if (smp::count <= HW_Q_COUNT)
        return this_shard_id();

    uint64_t current = rdtsc();

    auto busy_local = cores[this_shard_id()].busy_till->load();
    if (exec_time < GATE) {
        cores[this_shard_id()].busy_till->store(std::max(busy_local, current) + exec_time);
        return this_shard_id();
    }

    cores[0].mu->lock();
    uint64_t min = ULLONG_MAX;
    int min_index = -1;
 
    for (int i = HW_Q_COUNT; i < smp::count; i++) {
        //cores[i].mu->lock();
#ifdef BOOL_SCHED
	auto busy = cores[i].busy->load();
	if (busy)
	    continue;
	else {
	    min_index = i;
	    break;
	}
#endif
	auto busy_till = cores[i].busy_till->load();
        if (busy_till < min) {
            min = busy_till;
            min_index = i;
        }
        //cores[i].mu->unlock();
    }

    if (exec_time < LONG_FUNC && exec_time != -1) {
	if (min < rdtsc())
            min = rdtsc();
	// for hard sharding
        //min = ULLONG_MAX;
        for (int i = 0; i < HW_Q_COUNT; i++) {
#ifdef BOOL_SCHED
        auto busy = cores[i].busy->load();
        if (busy)
            continue;
        else {
            min_index = i;
            break;
        }
#endif
           //cores[i].mu->lock();
	   auto busy_till = cores[i].busy_till->load();
           if (busy_till < min) {
                min = busy_till;
                min_index = i;
           }
            //cores[i].mu->unlock();
        }
    }

#ifdef BOOL_SCHED
    if (min_index == -1)
        min_index = rand() % smp::count;
#endif
    if (exec_time < 1*2400000)
	min_index = this_shard_id();
	
    //cores[min_index].mu->lock();
    cores[min_index].busy->store(true);
    if (exec_time == 0)
        exec_time = 1;

    cores[min_index].q.push_back(call_id);
    auto busy_till = cores[min_index].busy_till->load();
    cores[min_index].busy_till->store(std::max(busy_till, current) + exec_time);
    //cores[min_index].mu->unlock();
    cores[0].mu->unlock();
    
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
        func_map[func] = (void*)function_states;
    }

    if (req_id == call_id && wf_map.find(func) == func_map.end()) {
        auto workflow_states = new wf_states;
	if (service == "complex.js")
	    workflow_states->fuse = true;
        wf_map[func] = (void*)workflow_states;
    }

    if (req_id == call_id)
	req_wf_map[req_id] = wf_map[func];

    auto workflow_states = (struct wf_states*)req_wf_map[req_id];
    auto function_states = (struct func_states*)func_map[func];
    size_t core;

    if (workflow_states->fuse) {
	if (req_id == call_id)
            core = get_core(workflow_states->exec_time, req_id, call_id, service);
	else
	    core = this_shard_id();
    } else
        core = get_core(function_states->exec_time->load(), req_id, call_id, service);

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

    cores[0].mu->lock();
    for (auto it = cores[cpu].q.begin(); it != cores[cpu].q.end(); it++)
	if (*it == call_id) {
            cores[cpu].q.erase(it);
	    break;
        }

    curr_req.erase(call_id);

    if (!curr_req.size()) {
        cores[cpu].busy->store(false);
        uint64_t current = rdtsc();

        cores[cpu].busy_till->store(current);
    }
    cores[0].mu->unlock();

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
	free(states);
	return make_ready_future<>();
    } else {
        if (states->prev_cpuid == cpu) {
            local_req_server().run_callback(call_id, service, ret);
            dispatch();
	    free(states);
            return make_ready_future<>();
	} else {
	    auto workflow_states = (struct wf_states*)req_wf_map[req_id];
	    req_wf_map.erase(req_id);
	    workflow_states->count++;
	    workflow_states->total_exec_time += (rdtsc() - wf_starts[req_id]);
	    workflow_states->exec_time = (workflow_states->total_exec_time/workflow_states->count);
	    if (workflow_states->count == 1000) {
	        workflow_states->count = 0;
		workflow_states->total_exec_time = 0;
	    }
	    wf_starts.erase(req_id);
	    if (workflow_states->fuse)
	        curr_wf.erase(req_id);

            sched_server.invoke_on(states->prev_cpuid, &scheduler::reply, req_id, call_id, service, ret).then([&] {
	    dispatch();});
	    free(states);
            return make_ready_future<>();
	}
    }
}
