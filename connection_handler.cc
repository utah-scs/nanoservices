#include "include/connection_handler.hh"
#include "include/req_server.hh"
#include "include/scheduler.hh"
#include "include/reply_builder.hh"

using namespace seastar;
void connection_handler::prepare_request()
{
    _request_args._command_args_count = _parser._args_count - 1;
    _request_args._command_args = std::move(_parser._args_list);
    _request_args._tmp_keys.clear();
    _request_args._tmp_key_values.clear();
    _request_args._tmp_key_scores.clear();
    _request_args._tmp_key_value_pairs.clear();
}

inline unsigned getcpu(const sstring& key) {
    return std::hash<sstring>()(key) % smp::count;
}

future<> connection_handler::handle(input_stream<char>& in, output_stream<char>& out) {
    _parser.init();

   // NOTE: The command is handled sequentially. The parser will control the lifetime
    // of every parameters for command.
    return in.consume(_parser).then([this, &in, &out] () -> future<> {
        switch (_parser._state) {
            case redis_protocol_parser::state::eof:
                printf("Parser eof\n");
            case redis_protocol_parser::state::error:
                //printf("Parser error\n");
                return make_ready_future<>();

            case redis_protocol_parser::state::ok:
            {
                prepare_request();
                switch (_parser._command) {
		    // Shredder adds JS command to run JavaScript functions
                    case redis_protocol_parser::command::js:
                        return get_local_sched()->new_req(std::move(std::ref(_request_args)), out);
                    case redis_protocol_parser::command::info:
                        return out.write(std::move(reply_builder::build_direct(msg_ok, msg_ok.size())));
                    case redis_protocol_parser::command::quit:
                        return out.write(std::move(reply_builder::build_direct(msg_ok, msg_ok.size())));

                    default:
                        //tracer.incr_number_exceptions();
                        return out.write(std::move(reply_builder::build_direct(msg_ok, msg_ok.size())));
                };
            }
            default:
                //tracer.incr_number_exceptions();
                return out.write(std::move(reply_builder::build_direct(msg_err, msg_ok.size())));
        };
        std::abort();
    });
}
