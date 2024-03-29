#pragma once
#include "seastarkv.hh"
#include "include/req_server.hh"
#include <deque>
#include <boost/thread/mutex.hpp>
#include <boost/make_shared.hpp>
#include <atomic>

using namespace seastar;
using namespace redis;

class scheduler;
extern distributed<scheduler> sched_server;
inline distributed<scheduler>& get_sched_server() {
    return sched_server;
}

inline scheduler* get_local_sched() {
    return &sched_server.local();
}

struct reply_states {
    bool local;
    size_t prev_cpuid;
    reply_states() {
    }
    ~reply_states() {}
};

struct js_reply {
    httpd::reply::status_type _status; 
    std::string _message;
};

struct local_reply_states : reply_states {
    promise<struct js_reply> res;
};

struct callback_states {
    v8::Global<v8::Function> callback;
    callback_states() {
    }
    ~callback_states() {}
};

struct func_states {
    std::atomic<unsigned> *count;
    std::atomic<uint64_t> *total_exec_time;
    std::atomic<int> *exec_time;
    func_states() {
	count = new std::atomic<unsigned>(0);
        exec_time = new std::atomic<int>(-1);
        total_exec_time = new std::atomic<uint64_t>( 0);
    }
    ~func_states() {}
};

struct wf_states {
    bool fuse;
    unsigned count;
    uint64_t total_exec_time;
    int exec_time;
    wf_states() {
	fuse = false;
	count = 0;
        exec_time = -1;
        total_exec_time = 0;
    }
    ~wf_states() {}
};

struct core_states {
    boost::shared_ptr<boost::mutex> mu;
    std::atomic<bool> *busy;
    std::atomic<uint64_t> *busy_till;
    //std::queue<task*> q;
    std::deque<sstring> q;
    std::unordered_map<std::string, task*> task_map;

    void init() {
        busy = new std::atomic<bool>(false);
	busy_till = new std::atomic<uint64_t>(0);
        mu = boost::make_shared<boost::mutex>();
    }
    core_states() {
    }
    ~core_states() {
    }
};

class scheduler {
private:
    std::unordered_map<std::string, void*> req_map;
    std::unordered_set<std::string> curr_req;
    std::unordered_set<std::string> curr_wf;
    std::unordered_map<std::string, uint64_t> wf_starts;

    uint64_t count = 0;
    bool big_core = false;

public:
    void dispatch();
    void start();
    void* get_req_states(std::string key);
    void del_req_states(std::string key);
    void set_req_states(std::string key, void* states);
    void* get_wf_states(std::string key);
    void set_wf_states(std::string key, void* states);

    future<> ping();
    future<> pong();
    future<> test_reply();

    future<> stop() {
      return make_ready_future<>();
    }
    scheduler() {};
    void new_service(std::string service);
    future<> new_req(std::unique_ptr<httpd::request> req, std::string req_id, sstring service, sstring function, std::string args, output_stream<char>& out);
    future<> run_func(size_t prev_cpu, std::string req_id, std::string call_id, std::string service, std::string function, std::string jsargs);
    future<> schedule(size_t prev_cpu, std::string req_id, std::string call_id, std::string service, std::string function, std::string jsargs);
    future<> reply(std::string req_id, std::string call_id, std::string service, std::string ret);
};

