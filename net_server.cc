#include "include/seastarkv.hh"
#include "include/net_server.hh"
#include "include/reply_builder.hh"
#include "include/req_server.hh"

distributed<network_server> net_server;

void network_server::start() {
     listen_options lo;
     lo.reuse_address = true;
     _listener = engine().listen(make_ipv4_address({_port}), lo);
     keep_doing([this] {
        return _listener->accept().then([this] (accept_result ar) mutable {
            connected_socket fd = std::move(ar.connection);
            socket_address addr = std::move(ar.remote_address);
            auto conn = make_lw_shared<connection>(std::move(fd), addr);
//            cout << "Connection from " << addr << " on core " << engine().cpu_id() << "\n";
            do_until([conn] { return conn->_in.eof(); }, [this, conn] {
                return conn->handler.handle(conn->_in, conn->_out).then([this, conn] {
                    return conn->_out.flush();
                });
            }).finally([this, conn] {
                return conn->_out.close().finally([conn]{});
            });
        });
    }).or_terminate();
}
