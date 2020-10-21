#pragma once
#include "seastarkv.hh"
#include "include/req_server.hh"

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

class scheduler {
private:
    std::unordered_map<std::string, void*> req_map;

public:
    void start();
    void* get_req_states(std::string key);
    void set_req_states(std::string key, void* states);

    future<> stop() {
      return make_ready_future<>();
    }
    scheduler() {};
    void new_service(std::string service);
    future<> new_req(std::unique_ptr<request> req, std::string req_id, sstring service, sstring function, std::string args, output_stream<char>& out);
    future<> run_func(size_t prev_cpu, std::string req_id, std::string call_id, std::string prev_service, std::string service, std::string function, std::string jsargs);
    future<> schedule(std::string req_id, std::string call_id, std::string prev_service, std::string service, std::string function, std::string jsargs);
    future<> reply(std::string req_id, std::string service, std::string ret);
};

