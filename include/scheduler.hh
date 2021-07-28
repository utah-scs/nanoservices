#pragma once
#include "seastarkv.hh"
#include "include/req_server.hh"
#include <queue>
#include <boost/thread/mutex.hpp>
#include <boost/make_shared.hpp>

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
    uint64_t start_time;
    int64_t exec_time;
    func_states() {
    }
    ~func_states() {}
};

struct core_states {
    boost::shared_ptr<boost::mutex> mu;
    bool busy = false;
    uint64_t busy_till = 0;
    std::queue<task*> q;

    core_states() {
        mu = boost::make_shared<boost::mutex>();
    }
    ~core_states() {
    }
};

/*struct cmp {
    bool operator() (struct wf_states* left, struct wf_states* right) const { 
        return (left->ts) < (right->ts);
    }
};
*/
class scheduler {
private:
    std::unordered_map<std::string, void*> req_map;
    std::unordered_map<std::string, void*> func_map;
    std::unordered_map<std::string, string> func_name_map;
    //std::set<struct wf_states*, cmp> wf_queue;
    uint64_t count = 0;
    bool big_core = false;

public:
    void dispatch();

    void start();
    void* get_req_states(std::string key);
    void set_req_states(std::string key, void* states);
    void* get_wf_states(std::string key);
    void set_wf_states(std::string key, void* states);

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

