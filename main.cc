#include "include/seastarkv.hh"
#include "include/net_server.hh"
#include "include/req_server.hh"
#include "include/db.hh"
#include "include/scheduler.hh"
#include <sys/resource.h>

#define PLATFORM "seastar"
#define VERSION "v1.0"
#define VERSION_STRING PLATFORM " " VERSION

using namespace seastar;
namespace bpo = boost::program_options;

int main(int argc, char** argv) {
    using namespace v8;

    V8::InitializeICUDefaultLocation(argv[0]);
    V8::InitializeExternalStartupData(argv[0]);
    std::unique_ptr<Platform> platform = platform::NewDefaultPlatform();
    V8::InitializePlatform(platform.get());
    V8::Initialize();

    seastar::app_template app;

    return app.run_deprecated(argc, argv, [&] {
	auto&& config = app.configuration();
       
	auto& net_server = get_net_server();
        auto& req_server = get_req_server();
        auto& sched_server = get_sched_server();

        engine().at_exit([&] { return net_server.stop(); });
        engine().at_exit([&] { return req_server.stop(); });
        engine().at_exit([&] { return sched_server.stop(); });

        return net_server.start().then([&] {
	        for (int i = 0; i < std::min<unsigned int>(smp::count, HW_Q_COUNT); i++) {
	            net_server.invoke_on(i, &network_server::start);
	        }
	        return make_ready_future<>();
        }).then([&] {
	        return sched_server.start().then([&] {return sched_server.invoke_on_all(&scheduler::start);});
	    }).then([&] {
	        return req_server.start().then([&] {
		        return req_server.invoke_on_all(&req_service::start).then([&req_server] {
	                req_server.invoke_on_all(&req_service::register_service, std::string("user.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("test.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("compose_post_service.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("media_service.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("text_service.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("url_shorten_service.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("user_mention_service.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("user_service.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("user_timeline_service.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("user-timeline.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("post.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("social_graph_service.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("unique_id_service.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("microbenchmark.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("microbenchmark2.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("microbenchmark3.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("microbenchmark4.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("home_timeline_service.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("write_home_timeline_service.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("post_storage.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("fanout.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("sched_bench.js"));
	                req_server.invoke_on_all(&req_service::register_service, std::string("complex.js"));
                    // Start JS thread on all cores
                    return req_server.invoke_on_all(&req_service::js);
		        });
            });
	    });
    });
    V8::Dispose();
    delete platform.get();
}
