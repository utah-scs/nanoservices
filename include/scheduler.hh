#pragma once
#include "seastarkv.hh"
#include "include/req_server.hh"
#include <queue>
#include <boost/thread/mutex.hpp>

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

struct wf_states {
    sstring name;
    uint64_t ts;
    uint64_t start_time;
    std::queue<task*> q;
    wf_states() {
    }
    ~wf_states() {}
};

struct wf_info {
    int64_t exec_time;
    wf_info() {
    }
    ~wf_info() {}
};

struct core_states {
    boost::mutex mu;
    uint64_t busy_till = 0;
    core_states() {
    }
    ~core_states() {}
};

class cmp {
public:
    bool operator() (struct wf_states* &left, struct wf_states* &right) { 
        return (left->ts) > (right->ts);
    }
};

class local_scheduler {
private:
    std::priority_queue<struct wf_states*, std::vector<struct wf_states*>, cmp> wf_queue;

public:
    local_scheduler() {};
   
    void dispatch(void) {
        auto workflow_states = wf_queue.top();
	if (!workflow_states) return;

        while (workflow_states->q.size()) {
            engine().add_task(workflow_states->q.front());
            workflow_states->q.pop();
        }
    };

     void new_wf(struct wf_states* workflow_states) {
        wf_queue.push(workflow_states);
        dispatch();
    };
 
    void complete_wf(std::string req_id) {
        wf_queue.pop();
        dispatch();
    };
};

class scheduler {
private:
    class local_scheduler local_sched;
    std::unordered_map<std::string, void*> req_map;
    std::unordered_map<std::string, void*> wf_map;
    std::unordered_map<std::string, void*> wf_info_map;
    uint64_t count = 0;
    bool big_core = false;

public:
    void start();
    void* get_req_states(std::string key);
    void set_req_states(std::string key, void* states);
    void* get_wf_states(std::string key);
    void set_wf_states(std::string key, void* states);
    void delete_wf_states(std::string req_id) {
	wf_map.erase(req_id);
	local_sched.complete_wf(req_id);
    };

    future<> stop() {
      return make_ready_future<>();
    }
    scheduler() {};
    void new_service(std::string service);
    void dispatch(std::string req_id, bool new_wf, bool complete_wf);
    future<> new_req(std::unique_ptr<httpd::request> req, std::string req_id, sstring service, sstring function, std::string args, output_stream<char>& out);
    future<> run_func(size_t prev_cpu, std::string req_id, std::string call_id, std::string service, std::string function, std::string jsargs);
    future<> schedule(size_t prev_cpu, std::string req_id, std::string call_id, std::string service, std::string function, std::string jsargs);
    future<> reply(std::string req_id, std::string call_id, std::string service, std::string ret);
};

