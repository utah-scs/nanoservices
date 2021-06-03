#include "include/seastarkv.hh"
#include "include/req_server.hh"
#include "include/net_server.hh"
#include "include/scheduler.hh"
#include "include/db.hh"
#include "v8/src/runtime/runtime.h"
#include <iostream>
#include <cstdio>
#include <condition_variable>
#include <thread> 
#include <fstream>
#include <chrono>
#include <string>
#include <functional>
#include <boost/uuid/detail/sha1.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <stdlib.h>

using namespace std;
using namespace seastar;
using namespace v8;
using namespace std::chrono;
using namespace shredder;
using namespace boost::uuids;

distributed<req_service> req_server;
extern mongocxx::pool *pool;

static const char* ToCString(const v8::String::Utf8Value& value) {
    return *value ? *value : "<string conversion failed>";
}

future<> req_service::start(void) {
    create_params.array_buffer_allocator =
        v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    isolate = Isolate::New(create_params);

    return make_ready_future<>();
}
future<> req_service::stop() {
    return make_ready_future<>();
}

future<> req_service::register_service(std::string service) {
    v8::Locker locker{isolate};              
    Isolate::Scope isolate_scope(isolate);
    {
        HandleScope handle_scope(isolate);

	v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
        // Set C++ bindings
        global->Set(
            v8::String::NewFromUtf8(isolate, "DBGet", v8::NewStringType::kNormal)
                .ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, db_get)
        );
        global->Set(
            v8::String::NewFromUtf8(isolate, "DBSet", v8::NewStringType::kNormal)
                .ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, db_set)
        );
        global->Set(
            v8::String::NewFromUtf8(isolate, "DBDel", v8::NewStringType::kNormal)
                .ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, db_del)
        );
        global->Set(
            v8::String::NewFromUtf8(isolate, "print", v8::NewStringType::kNormal)
                .ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, js_print)
        );
        global->Set(
            v8::String::NewFromUtf8(isolate, "GetHashTable", v8::NewStringType::kNormal)
                .ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, get_hash_table)
        );
        global->Set(
            v8::String::NewFromUtf8(isolate, "Call", v8::NewStringType::kNormal)
                .ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, call_function)
        );
	global->Set(
            v8::String::NewFromUtf8(isolate, "NewDB", v8::NewStringType::kNormal)
                .ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, new_database)
        );
        global->Set(
            v8::String::NewFromUtf8(isolate, "Reply", v8::NewStringType::kNormal)
                .ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, shredder::reply)
        );
        global->Set(
            v8::String::NewFromUtf8(isolate, "Sha1", v8::NewStringType::kNormal)
                .ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, shredder::sha1)
        );
        global->Set(
            v8::String::NewFromUtf8(isolate, "Base64Decode", v8::NewStringType::kNormal)
                .ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, shredder::base64_decode)
        );
	global->Set(
            v8::String::NewFromUtf8(isolate, "ServiceName", v8::NewStringType::kNormal)
                .ToLocalChecked(),
	    String::NewFromUtf8(isolate, service.c_str(), NewStringType::kNormal)
                .ToLocalChecked()
        );
        global->Set(
            v8::String::NewFromUtf8(isolate, "MongoGet", v8::NewStringType::kNormal)
                .ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, shredder::mongo_get)
        );
        global->Set(
            v8::String::NewFromUtf8(isolate, "CoreID", v8::NewStringType::kNormal)
                .ToLocalChecked(),
            v8::Integer::NewFromUnsigned(isolate, engine().cpu_id())
        );

        Local<Context> c = Context::New(isolate, NULL, global);
        Context::Scope contextScope(c);

	Local<String> source;
        // Read the JS script file.
        if (!read_file(isolate, "services/" + service).ToLocal(&source)) {
            fprintf(stderr, "Error reading file\n");
        }

        Local<Script> s =
          Script::Compile(c, source).ToLocalChecked();

        Local<Value> result;
        if (!s->Run(c).ToLocal(&result)) {
          printf("run script error\n");
        }

        ctx_map[service] = unallocated_ctx;
        contexts[unallocated_ctx].Reset(isolate, c);
	unallocated_ctx++;
    }
    create_db(service);
    get_local_sched()->new_service(service);
    return make_ready_future<>();
}

void req_service::run_callback(std::string call_id, std::string service, sstring ret) {
    v8::Locker locker{isolate};
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);

    // Switch to V8 context of this service
    Local<Context> context = Local<Context>::New(isolate, contexts[ctx_map[service]]);
    Context::Scope context_scope(context);
    current_context = &context;

    auto key = service + call_id + "cb";
    auto states = (struct callback_states*)get_local_sched()->get_req_states(key);
    Local<Function> callback = Local<Function>::New(isolate, states->callback);
    const int argc = 2;
    Local<Value> argv[argc];
    argv[0] = v8::Null(isolate);
    argv[1] = String::NewFromUtf8(isolate, ret.c_str(), NewStringType::kNormal)
                                 .ToLocalChecked();;

    Local<Value> result;
    if (!callback->Call(context, context->Global(), argc, argv).ToLocal(&result)) {

    } else {
    }
}

void req_service::run_func(std::string req_id, std::string call_id, std::string service, std::string function, std::string jsargs) {
    v8::Locker locker{isolate};              
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);

    // Switch to V8 context of this service
    Local<Context> context = Local<Context>::New(isolate, contexts[ctx_map[service]]);
    Context::Scope context_scope(context);
    current_context = &context;

    Local<Function> process_fun;
    Local<String> process_name = String::NewFromUtf8(isolate, function.c_str(), NewStringType::kNormal)
                                 .ToLocalChecked();
    Local<Value> process_val;
    if (!context->Global()->Get(context, process_name).ToLocal(&process_val) ||
        !process_val->IsFunction()) {
         printf("get function fail\n");
    }
    process_fun = Local<Function>::Cast(process_val);
 
    Local<Value> result;

    const int argc = 3;
    Local<Value> argv[argc];
    argv[0] = String::NewFromUtf8(isolate, req_id.c_str(), NewStringType::kNormal)
                                 .ToLocalChecked();
    argv[1] = String::NewFromUtf8(isolate, call_id.c_str(), NewStringType::kNormal)
                                 .ToLocalChecked();
    argv[2] = String::NewFromUtf8(isolate, jsargs.c_str(), NewStringType::kNormal)
                                 .ToLocalChecked();

    if (!process_fun->Call(context, context->Global(), argc, argv).ToLocal(&result)) {

    } else {
    }
}

// Run JavaScript function
void req_service::js_req(std::string req_id, sstring service, 
		sstring function, std::string args, output_stream<char>& out) {

    v8::Locker locker{isolate};              
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);

    // Switch to V8 context of this service
    std::string serv(service.c_str());
    Local<Context> context = Local<Context>::New(isolate, contexts[ctx_map[serv]]);
    Context::Scope context_scope(context);
    current_context = &context;

    Local<Function> process_fun;
    Local<String> process_name =
        String::NewFromUtf8(isolate, function.c_str(), NewStringType::kNormal)
            .ToLocalChecked();
    Local<Value> process_val;
    if (!context->Global()->Get(context, process_name).ToLocal(&process_val) ||
        !process_val->IsFunction()) {
         printf("get function %s fail\n", function.c_str());
    }
    process_fun = Local<Function>::Cast(process_val);
    
    Local<Value> result;

    const int argc = 3;
    Local<Value> argv[argc];
    argv[0] = v8::String::NewFromUtf8(isolate, req_id.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
    argv[1] = v8::String::NewFromUtf8(isolate, req_id.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
    argv[2] = v8::String::NewFromUtf8(isolate, args.c_str(), v8::NewStringType::kNormal).ToLocalChecked();

    if (!process_fun->Call(context, context->Global(), argc, argv).ToLocal(&result)) {
         auto cstr = "error\n";
         get_local_sched()->reply(req_id, req_id, service, to_sstring(cstr));
    } else {
    }
}

enum AllocationSpace {
  NEW_SPACE,   // Semispaces collected with copying collector.
  OLD_SPACE,   // May contain pointers to new space.
  CODE_SPACE,  // No pointers to new space, marked executable.
  MAP_SPACE,   // Only and all map objects.
  LO_SPACE,    // Promoted large objects.

  FIRST_SPACE = NEW_SPACE,
  LAST_SPACE = LO_SPACE,
  FIRST_PAGED_SPACE = OLD_SPACE,
  LAST_PAGED_SPACE = MAP_SPACE
};

// The JS thread. Keep this thread although it's doing nothing, because 
// performance is better with this thread around, maybe because it keeps
// some V8 states from garbage collectioned or something.
future<> req_service::js() {
    async([this] 
    {
        v8::Locker locker{isolate};              
        Isolate::Scope isolate_scope(isolate);

        while (true) {
            HandleScope handle_scope(isolate);
            sem.wait(1).get();
        }
    });
    return make_ready_future<>();
}

namespace shredder {
// C++ binding for JS functions to get data from hashtable
void db_get(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto ctx = args.GetIsolate()->GetCurrentContext();

    v8::String::Utf8Value str(args.GetIsolate(), args[0]);
    auto name = std::string(*str);
    auto db = get_db(name);

    uint32_t key;
    if (args[1]->IsString()) {
        v8::String::Utf8Value s(args.GetIsolate(), args[1]);
	auto k = std::string(*s);
	std::hash<std::string> hasher;
	auto hashed = hasher(k);
	key = static_cast<int>(hashed % numeric_limits<uint32_t>::max());
    } else {
        key = args[1]->Uint32Value(ctx).ToChecked();
    }

    db_val* val = db->ht_get(key);
    if (!val) {
        val = (db_val*)malloc(sizeof(db_val));
	val->data = NULL;
        val->length = 0;
    }
    
    Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(args.GetIsolate(), val->data, val->length);

    args.GetReturnValue().Set(ab);
    free(val);
}

// C++ binding for JS functions to set data to hashtable
void db_set(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto ctx = args.GetIsolate()->GetCurrentContext();
    v8::String::Utf8Value str(args.GetIsolate(), args[0]);
    auto name = std::string(*str);
    auto db = get_db(name);

    uint32_t key;
    if (args[1]->IsString()) {
        v8::String::Utf8Value s(args.GetIsolate(), args[1]);
	auto k = std::string(*s);
	std::hash<std::string> hasher;
	auto hashed = hasher(k);
	key = static_cast<int>(hashed % numeric_limits<uint32_t>::max());
    } else {
        key = args[1]->Uint32Value(ctx).ToChecked();
    }

    auto content = args[2].As<v8::ArrayBuffer>()->Externalize();

    auto version = args[3]->Uint32Value(ctx).ToChecked();

    db_val* val = (db_val*)malloc(sizeof(db_val));
    auto tmp = malloc(content.ByteLength() + sizeof(int));
    auto p = (int*)tmp;
    *p = rand();
    p++;
    memcpy(p, content.Data(), content.ByteLength());
    val->data = tmp;
    val->length = content.ByteLength() + sizeof(int);
    val->key = key;

    free(content.Data());

    if (db->ht_set(val, version)) {
        args.GetReturnValue().Set(
            v8::String::NewFromUtf8(args.GetIsolate(), "ok",
                                    v8::NewStringType::kNormal).ToLocalChecked());
    } else {
         args.GetReturnValue().Set(
            v8::String::NewFromUtf8(args.GetIsolate(), "abort",
                                    v8::NewStringType::kNormal).ToLocalChecked());
    }
}

void db_del(const v8::FunctionCallbackInfo<v8::Value>& args) {
}

// C++ binding for JS functions to print messages
void js_print(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate * isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);

    v8::String::Utf8Value str(args.GetIsolate(), args[0]);
    const char* cstr = ToCString(str);
    std::cout << cstr << '\n';
}

// C++ binding for JS functions to get hashtable
void get_hash_table(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate * isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);

    v8::String::Utf8Value str(args.GetIsolate(), args[0]);
    auto name = std::string(*str);
    auto db = get_db(name);

    auto table = db->get_table_direct(); 
    Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(args.GetIsolate(), table, 1024*1024);
    args.GetReturnValue().Set(ab);
}

void call_function(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate * isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);
    auto context = args.GetIsolate()->GetCurrentContext();

    v8::String::Utf8Value num1(isolate, args[0]);
    auto req_id = std::string(*num1);

    v8::String::Utf8Value num2(isolate, args[1]);
    auto caller = std::string(*num2);

    v8::String::Utf8Value str1(args.GetIsolate(), args[2]);
    auto service = std::string(*str1);
    v8::String::Utf8Value str2(args.GetIsolate(), args[3]);
    auto function = std::string(*str2);

    Local<Function> callback = Local<Function>::Cast(args[4]);

    v8::String::Utf8Value str3(args.GetIsolate(), args[5]);
    auto jsargs = std::string(*str3);

    Local<String> tmp = String::NewFromUtf8(isolate, "ServiceName", NewStringType::kNormal)
                                            .ToLocalChecked();
    v8::String::Utf8Value str(isolate, context->Global()->Get(context, tmp).ToLocalChecked());
    auto local_service = std::string(*str);
    auto states = new callback_states;

    auto gen = random_generator();
    auto callee = to_string(gen());

    states->callback.Reset(isolate, callback);
    auto key = service + callee + "cb";
    get_local_sched()->set_req_states(key, (void*)states);

    get_local_sched()->schedule(req_id, caller, callee, local_service, service, function, jsargs);
}

void new_database(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate * isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);

    v8::String::Utf8Value str(isolate, args[0]);
    auto ret = create_db(std::string(*str));
    args.GetReturnValue().Set(v8::Boolean::New(isolate, ret));
}

void reply(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate * isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);

    v8::String::Utf8Value num1(isolate, args[0]);
    auto req_id = std::string(*num1);

    v8::String::Utf8Value num2(isolate, args[1]);
    auto call_id = std::string(*num2);
    v8::String::Utf8Value str1(isolate, args[2]);
    auto service = std::string(*str1);
    v8::String::Utf8Value str2(isolate, args[3]);
    auto ret = std::string(*str2);
    get_local_sched()->reply(req_id, call_id, service, ret);
}

void sha1(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate * isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);    

    v8::String::Utf8Value arg(isolate, args[0]);
    auto str = std::string(*arg);

    boost::uuids::detail::sha1 s;
    char hash[41] = {0};
    s.process_bytes(str.c_str(), str.size());
    unsigned int digest[5];
    s.get_digest(digest);
    for (int i = 0; i < 5; i++)
    {
        std::sprintf(hash + (i << 3), "%08x", digest[i]);
    }

    args.GetReturnValue().Set(
    v8::String::NewFromUtf8(args.GetIsolate(), hash,
                            v8::NewStringType::kNormal).ToLocalChecked());
}

void base64_decode(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate * isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);

    v8::String::Utf8Value arg(isolate, args[0]);
    auto tmp = std::string(*arg);
    std::string dest;
    dest.resize(boost::beast::detail::base64::decoded_size(tmp.size()));
    boost::beast::detail::base64::decode(&dest[0], tmp.c_str(), tmp.size());

    args.GetReturnValue().Set(
    v8::String::NewFromUtf8(args.GetIsolate(), dest.c_str(),
                            v8::NewStringType::kNormal).ToLocalChecked());
}

void mongo_get(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate * isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);    

    v8::String::Utf8Value arg0(isolate, args[0]);
    auto db = std::string(*arg0);
    v8::String::Utf8Value arg1(isolate, args[1]);
    auto collection = std::string(*arg1);
    v8::String::Utf8Value arg2(isolate, args[2]);
    auto username= std::string(*arg2);

    auto cli = pool->acquire();
    auto col = cli->database(db)[collection];
    bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result =
      col.find_one(bsoncxx::builder::stream::document{} 
		      << "username" 
		      << username 
		      << bsoncxx::builder::stream::finalize);

    args.GetReturnValue().Set(
    v8::String::NewFromUtf8(args.GetIsolate(), bsoncxx::to_json(*maybe_result).c_str(),
                            v8::NewStringType::kNormal).ToLocalChecked());
}
}
