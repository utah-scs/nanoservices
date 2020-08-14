#pragma once
#include "seastarkv.hh"

using namespace seastar;
using namespace redis;

class connection_handler {
private:
    http_request_parser _parser;
    args_collection _request_args;

public:
    connection_handler() {};
    void prepare_request();
    future<> handle(input_stream<char>& in, output_stream<char>& out);
};

