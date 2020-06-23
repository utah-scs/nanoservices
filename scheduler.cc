#include "include/scheduler.hh"
#include "include/req_server.hh"

using namespace seastar;

distributed<scheduler> sched_server;

void scheduler::start() {
}

future<> scheduler::new_req(args_collection& args, output_stream<char>* out) {
    size_t req_id = (size_t)atoi(args._command_args[0].c_str()); 
    auto new_states = new req_states;
    new_states->local = true;
    new_states->out = out;
    req_map[req_id] = new_states;
    return local_req_server().js_req(std::move(std::ref(args)), out);
}

future<> scheduler::schedule(const v8::FunctionCallbackInfo<v8::Value>& args) {
    return local_req_server().run_func(args);
}

future<> scheduler::reply(size_t req_id, std::string ret) {
    auto states = req_map[req_id];
    if (states->local)
        states->out->write(ret);

    req_map.erase(req_id);
    free(states);
    return make_ready_future<>();
}
