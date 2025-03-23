all : shredder
CC=g++

CFLAGS=-O2 -std=gnu++20 -U_FORTIFY_SOURCE -Wno-maybe-uninitialized -DSEASTAR_SSTRING -DSEASTAR_SCHEDULING_GROUPS_COUNT=16 -Wno-error=unused-result -DSEASTAR_API_LEVEL=2 -DFMT_LOCALE -pthread -I/usr/local/include/bsoncxx/v_noabi -I/usr/local/include/mongocxx/v_noabi -I/usr/local/include/libmongoc-1.0 -I/usr/local/include/libbson-1.0 -L/usr/local/lib -I./seastar/include -I./seastar/include/seastar -I./seastar/build/release/gen/include -I/usr/include/p11-kit-1 -Iv8/ -Iv8/include -Iv8/include/libplatform/ -Iv8/third_party/icu/source/i18n -Iv8/third_party/icu/source/common -I./ -L/usr//usr/lib/x86_64-linux-gnu ./seastar/build/release/libseastar.a /usr/lib/x86_64-linux-gnu/libboost_program_options.so /usr/lib/x86_64-linux-gnu/libboost_thread.so /usr/lib/x86_64-linux-gnu/libcares.so /usr/lib/x86_64-linux-gnu/libcryptopp.so /usr/lib/x86_64-linux-gnu/libfmt.so /usr/lib/x86_64-linux-gnu/liburing.so /usr/lib/x86_64-linux-gnu/liburing.so -ldl -lrt /usr/lib/x86_64-linux-gnu/libboost_filesystem.so /usr/lib/x86_64-linux-gnu/libboost_thread.so /usr/lib/x86_64-linux-gnu/libsctp.so /usr/lib/x86_64-linux-gnu/libnuma.so -latomic -lgnutls -pthread -lgmp -lunistring -lidn2 -latomic -lhogweed -lgmp -lnettle -ltasn1 -lp11-kit -lprotobuf -pthread -lz -lhwloc -lm -ludev -lltdl -lpthread -ldl -lyaml-cpp -llz4 -fcoroutines -fpermissive 

DEPS= include/connection_handler.hh include/net_server.hh include/redis_protocol_parser.hh include/reply_builder.hh include/scheduler.hh include/db.hh include/req_server.hh include/seastarkv.hh 
%.o: %.cc $(DEPS) 
	$(CC) -c $< $(CFLAGS) -o $@

shredder : main.o connection_handler.o net_server.o req_server.o db.o scheduler.o
	$(CC) -o $@ $^ $(CFLAGS) -Lv8/out/x64.release/obj -Wl,--start-group -lv8_monolith -Wl,--end-group -ldl -lrt -pthread

.PHONY: clean

clean:
	rm -f *.o shredder shredder.d
