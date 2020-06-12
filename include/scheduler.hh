#pragma once
#include "seastarkv.hh"

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

class scheduler {
private:

public:
    void start();

    future<> stop() {
      return make_ready_future<>();
    }
    scheduler() {};
    future<> schedule(const v8::FunctionCallbackInfo<v8::Value>& args);
};

