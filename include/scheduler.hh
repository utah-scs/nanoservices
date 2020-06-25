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

struct req_states {
    size_t prev_cpuid;
    bool local;
    output_stream<char>* out;
    v8::Global<v8::Function> callback;
    req_states() {
        out = NULL;
    }
    ~req_states() {}
};

class scheduler {
private:
    std::unordered_map<size_t, struct req_states*> req_map;

public:
    void start();

    future<> stop() {
      return make_ready_future<>();
    }
    scheduler() {};
    future<> new_req(args_collection& args, output_stream<char>* out);
    future<> schedule(const v8::FunctionCallbackInfo<v8::Value>& args);
    future<> reply(size_t req_id, sstring ret);
};

