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
auto instance = new mongocxx::instance();
mongocxx::pool *pool;

int main(int argc, char** argv) {
    using namespace v8;
    char* tmp[3];
    const char* a1 = "seastarkv";
    const char* a2 = "--max-old-space-size=8192";
    const char* a3 = "--use-strict";
    //const char* a3 = "--lite-mode";
    tmp[0] = (char*)a1;
    tmp[1] = (char*)a2;
    tmp[2] = (char*)a3;
    char** opt = (char**)&tmp;

    V8::InitializeICUDefaultLocation(argv[0]);
    V8::InitializeExternalStartupData(argv[0]);
    std::unique_ptr<Platform> platform = platform::NewDefaultPlatform();
    V8::InitializePlatform(platform.get());
    V8::Initialize();

    int c = 3;
    V8::SetFlagsFromCommandLine(&c, opt, true);
    v8::internal::FLAG_expose_gc = true;
    v8::internal::FLAG_allow_natives_syntax = true;
    v8::internal::FLAG_max_old_space_size = 8192;

    seastar::app_template app;
    app.add_options()("mongodb", bpo::value<sstring>()->default_value("h0.nano.sandstorm-pg0.utah.cloudlab.us:30012"), "MongoDB Server port");

    return app.run_deprecated(argc, argv, [&] {
	auto&& config = app.configuration();
        auto mongodb = config["mongodb"].as<sstring>();
	mongocxx::uri uri{("mongodb://" + mongodb).c_str()};
	pool = new mongocxx::pool(uri);
       
	auto& net_server = get_net_server();
        auto& req_server = get_req_server();
        auto& sched_server = get_sched_server();

        engine().at_exit([&] { return net_server.stop(); });
        engine().at_exit([&] { return req_server.stop(); });
        engine().at_exit([&] { return sched_server.stop(); });

        return net_server.start().then([&] {
            return net_server.invoke_on_all(&network_server::start);
        }).then([&] {
	    return sched_server.start().then([&] {return sched_server.invoke_on(0, &scheduler::start);});
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
	            req_server.invoke_on_all(&req_service::register_service, std::string("post.js"));
	            req_server.invoke_on_all(&req_service::register_service, std::string("social_graph_service.js"));
	            req_server.invoke_on_all(&req_service::register_service, std::string("unique_id_service.js"));
	            req_server.invoke_on_all(&req_service::register_service, std::string("microbenchmark.js"));
	            req_server.invoke_on_all(&req_service::register_service, std::string("microbenchmark2.js"));
	            req_server.invoke_on_all(&req_service::register_service, std::string("microbenchmark3.js"));
	            req_server.invoke_on_all(&req_service::register_service, std::string("microbenchmark4.js"));
	            req_server.invoke_on_all(&req_service::register_service, std::string("home_timeline_service.js"));
	            req_server.invoke_on_all(&req_service::register_service, std::string("post_storage.js"));
                    // Start JS thread on all cores
                    return req_server.invoke_on_all(&req_service::js);
		});
            });
	});
    });
    V8::Dispose();
    V8::ShutdownPlatform();
    delete platform.get();
}
