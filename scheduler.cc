#include "include/scheduler.hh"
#include "include/reply_builder.hh"
#include "include/req_server.hh"

using namespace seastar;

distributed<scheduler> sched_server;

void scheduler::start() {
}

void scheduler::new_service(std::string service) {
    std::unordered_map<size_t, struct req_states*>* map = new std::unordered_map<size_t, struct req_states*>();
    req_map[service] = map;
}

struct req_states* scheduler::get_req_states(std::string service, size_t req_id) {
    auto map = req_map[service];
    return (*map)[req_id];
}

future<> scheduler::new_req(args_collection& args, output_stream<char>* out) {
    size_t req_id = (size_t)atoi(args._command_args[0].c_str()); 
    auto service = std::string(args._command_args[1].c_str());
    auto new_states = new req_states;
    new_states->local = true;
    new_states->out = out;
    (*req_map[service])[req_id] = new_states;

    return local_req_server().js_req(std::move(std::ref(args)), out);
}

future<> scheduler::run_func(size_t cpuid, size_t req_id, std::string prev_service, std::string service, std::string function, std::string jsargs) {
    auto new_states = new req_states;
    new_states->local = false;
    new_states->prev_cpuid = cpuid;
    new_states->prev_service = prev_service;

    (*req_map[service])[req_id] = new_states;
    return local_req_server().run_func(req_id, service, function, jsargs);
}

future<> scheduler::schedule(size_t req_id, std::string prev_service, std::string service, std::string function, std::string jsargs) {

    //return run_func(engine().cpu_id(), req_id, prev_service, service, function, jsargs);
    return sched_server.invoke_on(engine().cpu_id() + 1, &scheduler::run_func, engine().cpu_id(), req_id, prev_service, service, function, jsargs);
}

future<> scheduler::reply(size_t req_id, std::string service, sstring ret) {
    auto map = req_map[service];
    auto states = (*map)[req_id];
    map->erase(req_id);
    if (states->local) {
        return states->out->write(std::move(reply_builder::build_direct(ret, ret.size())))
	    .then([&states] () {
	        states->out->flush();
                free(states);
	    });
    } else {
        return req_server.invoke_on(states->prev_cpuid, &req_service::run_callback, req_id, states->prev_service, ret)
	    .then([&states] () {
            free(states);
        });
    }
}
