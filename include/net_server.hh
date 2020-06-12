#pragma once
#include "seastarkv.hh"
#include "connection_handler.hh"

using namespace v8;
using namespace seastar;

class network_server;
extern distributed<network_server> net_server;
inline distributed<network_server>& get_net_server() {
    return net_server;
}

class network_server {
private:
    lw_shared_ptr<server_socket> _listener;
    uint16_t _port = 11211;

    struct connection {
        connected_socket _socket;
        socket_address _addr;
        input_stream<char> _in;
        output_stream<char> _out;
        connection_handler handler;
        connection(connected_socket&& socket, socket_address addr)
            : _socket(std::move(socket))
              , _addr(addr)
              , _in(_socket.input())
              , _out(_socket.output())
        {
        }
        ~connection() {
        }
    };

    seastar::gate _request_gate;

public:
    network_server()
    {
    }

   void start();

    future<> stop() {
      return make_ready_future<>();
    }
};
