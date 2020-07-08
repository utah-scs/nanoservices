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

using namespace std;
using namespace seastar;
using namespace v8;
using namespace std::chrono;

distributed<req_service> req_server;

static const char* ToCString(const v8::String::Utf8Value& value) {
    return *value ? *value : "<string conversion failed>";
}

future<> req_service::start() {
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
            v8::FunctionTemplate::New(isolate, reply)
        );
	global->Set(
            v8::String::NewFromUtf8(isolate, "ServiceName", v8::NewStringType::kNormal)
                .ToLocalChecked(),
	    String::NewFromUtf8(isolate, service.c_str(), NewStringType::kNormal)
                .ToLocalChecked()
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

future<> req_service::run_callback(std::string req_id, std::string service, sstring ret) {
    v8::Locker locker{isolate};
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);

    // Switch to V8 context of this service
    Local<Context> context = Local<Context>::New(isolate, contexts[ctx_map[service]]);
    Context::Scope context_scope(context);
    current_context = &context;

    auto key = service + req_id + "cb";
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
    return make_ready_future<>();
}

future<> req_service::run_func(std::string req_id, std::string service, std::string function, std::string jsargs) {
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

    const int argc = 2;
    Local<Value> argv[argc];
    argv[0] = String::NewFromUtf8(isolate, req_id.c_str(), NewStringType::kNormal)
                                 .ToLocalChecked();
    argv[1] = String::NewFromUtf8(isolate, jsargs.c_str(), NewStringType::kNormal)
                                 .ToLocalChecked();

    if (!process_fun->Call(context, context->Global(), argc, argv).ToLocal(&result)) {

    } else {
    }
    return make_ready_future<>();
}

// Run JavaScript function
future<> req_service::js_req(args_collection& args, output_stream<char>* out) {
    v8::Locker locker{isolate};              
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);

    auto req = make_lw_shared<rqst>(args);
    if (req->args._command_args_count < 3) {
        sstring tmp = to_sstring(msg_syntax_err);
        auto result = reply_builder::build_direct(tmp, tmp.size());
        return out->write(std::move(result));
    }      

    auto req_id = std::string(req->args._command_args[0].c_str());

    auto service = std::string(req->args._command_args[1].c_str());
    // Switch to V8 context of this service
    Local<Context> context = Local<Context>::New(isolate, contexts[ctx_map[service]]);
    Context::Scope context_scope(context);
    current_context = &context;

    sstring& name = req->args._command_args[2];

    Local<Function> process_fun;
    Local<String> process_name =
        String::NewFromUtf8(isolate, name.c_str(), NewStringType::kNormal)
            .ToLocalChecked();
    Local<Value> process_val;
    if (!context->Global()->Get(context, process_name).ToLocal(&process_val) ||
        !process_val->IsFunction()) {
         printf("get function %s fail\n", name.c_str());
    }
    process_fun = Local<Function>::Cast(process_val);
 
    Local<Value> result;

    const int argc = req->args._command_args_count -2;
    Local<Value> argv[argc];
    argv[0] = v8::String::NewFromUtf8(isolate, req->args._command_args[0].c_str(), v8::NewStringType::kNormal).ToLocalChecked();
    for (int i = 1; i < argc; i++) {
	argv[i] = v8::String::NewFromUtf8(isolate, req->args._command_args[i+2].c_str(), v8::NewStringType::kNormal)
          .ToLocalChecked();
    }

    if (!process_fun->Call(context, context->Global(), argc, argv).ToLocal(&result)) {
         auto cstr = "error\n";
         get_local_sched()->reply(req_id, service, to_sstring(cstr));
    } else {
    }
    return make_ready_future<>();
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

// C++ binding for JS functions to get data from hashtable
void db_get(const v8::FunctionCallbackInfo<v8::Value>& args) {
    db_val ret;
    db_val* ret_p = &ret;

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
        val->length = 0;
    }
    
    Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(args.GetIsolate(), val->data, val->length);

    args.GetReturnValue().Set(ab);
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
    db_val* val = (db_val*)malloc(sizeof(db_val));
    val->data = content.Data();
    val->length = content.ByteLength();
    val->key = key;

    db->ht_set(val);

    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(args.GetIsolate(), "ok\n",
                                v8::NewStringType::kNormal).ToLocalChecked());

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

    v8::String::Utf8Value num(isolate, args[0]);
    auto req_id = std::string(*num);

    v8::String::Utf8Value str1(args.GetIsolate(), args[1]);
    auto service = std::string(*str1);
    v8::String::Utf8Value str2(args.GetIsolate(), args[2]);
    auto function = std::string(*str2);

    Local<Function> callback = Local<Function>::Cast(args[3]);

    v8::String::Utf8Value str3(args.GetIsolate(), args[4]);
    auto jsargs = std::string(*str3);

    Local<String> tmp = String::NewFromUtf8(isolate, "ServiceName", NewStringType::kNormal)
                                            .ToLocalChecked();
    v8::String::Utf8Value str(isolate, context->Global()->Get(context, tmp).ToLocalChecked());
    auto local_service = std::string(*str);
    auto states = new callback_states;

    states->callback.Reset(isolate, callback);
    auto key = service + req_id + "cb";
    get_local_sched()->set_req_states(key, (void*)states);

    get_local_sched()->schedule(req_id, local_service, service, function, jsargs);
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

    v8::String::Utf8Value num(isolate, args[0]);
    auto req_id = std::string(*num);
    v8::String::Utf8Value str1(isolate, args[1]);
    auto service = std::string(*str1);
    v8::String::Utf8Value str2(isolate, args[2]);
    auto ret = to_sstring(ToCString(str2));
    get_local_sched()->reply(req_id, service, ret);
}
