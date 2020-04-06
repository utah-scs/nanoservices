#include "include/scheduler.hh"
#include "include/req_server.hh"

using namespace seastar;

distributed<scheduler> sched_server;

void scheduler::start() {
}

future<> scheduler::schedule(function_args args, int tid) {
    return local_req_server().run_func(args, tid);
}
