#include "include/scheduler.hh"
#include "include/reply_builder.hh"
#include "include/req_server.hh"

using namespace seastar;

distributed<scheduler> sched_server;

void scheduler::start() {
}

void scheduler::new_service(std::string service) {
}

void* scheduler::get_req_states(std::string key) {
    return req_map[key];
}

void scheduler::set_req_states(std::string key, void* states) {
    req_map[key] = states;
}

future<> scheduler::new_req(args_collection& args, output_stream<char>& out) {
    auto req_id = std::string(args._command_args[0].c_str()); 
    auto service = std::string(args._command_args[1].c_str());
    auto key = service + req_id + "reply";
    auto new_states = new reply_states;
    new_states->local = true;
    auto f = new_states->res.get_future();
    req_map[key] = (void*)new_states;

    local_req_server().js_req(std::move(std::ref(args)), out);
    return f.then([&out, &new_states] (auto&& f) {
		        out.write(f).then([&new_states] {
					free(new_states);
					});
			});
}

future<> scheduler::run_func(size_t cpuid, std::string req_id, std::string prev_service, 
		             std::string service, std::string function, std::string jsargs) {
    auto new_states = new reply_states;
    new_states->local = false;
    new_states->prev_cpuid = cpuid;
    auto key = service + req_id + "reply";
    req_map[key] = (void*)new_states;

    return local_req_server().run_func(req_id, service, function, jsargs);
}

future<> scheduler::schedule(std::string req_id, std::string prev_service, std::string service, 
		             std::string function, std::string jsargs) {
    return run_func(engine().cpu_id(), req_id, prev_service, service, function, jsargs);
    return sched_server.invoke_on(engine().cpu_id() + 1 , &scheduler::run_func, engine().cpu_id(), req_id, 
		                  prev_service, service, function, jsargs);
}

future<> scheduler::reply(std::string req_id, std::string service, sstring ret) {
    auto key = service + req_id + "reply";
    auto states = (struct reply_states*)req_map[key];

    req_map.erase(key);

    if (states->local) {
        states->res.set_value(std::move(reply_builder::build_direct(ret, ret.size())));
	return make_ready_future<>();
        //return states->out->write(std::move(reply_builder::build_direct(msg_ok, msg_ok.size())))
	//    .then([&states] () {
        //        free(states);
	//    });
    } else {
        return req_server.invoke_on(states->prev_cpuid, &req_service::run_callback, req_id, service, ret)
	    .then([&states] () {
            free(states);
        });
    }
}
