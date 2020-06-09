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

inline uint32_t get_hash(uint32_t key, uint32_t seed) {
    uint32_t hash = key;
    hash = hash ^ seed;
    hash = ~hash + (hash << 15);  // hash = (hash << 15) - hash - 1;
    hash = hash ^ (hash >> 12);
    hash = hash + (hash << 2);
    hash = hash ^ (hash >> 4);
    hash = hash * 2057;  // hash = (hash + (hash << 3)) + (hash << 11);
    hash = hash ^ (hash >> 16);
    return hash & 0x3fffffff;
}

future<int> req_service::get_tid(void) {
    return make_ready_future<int>(current_tid);
}

int req_service::get_tid_direct(void) {
    return current_tid;
}

future<> req_service::run_func(const v8::FunctionCallbackInfo<v8::Value>& args, int tid) {
    v8::Locker locker{isolate};              
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);

    current_tid = tid;
    
    // Switch to V8 context of this tenant
    Local<Context> context = Local<Context>::New(isolate, contexts[current_tid]);
    Context::Scope context_scope(context);
    current_context = &context;

    Local<Function> process_fun;
    Local<String> process_name = Local<String>::Cast(args[0]);
    Local<Value> process_val;
    if (!context->Global()->Get(context, process_name).ToLocal(&process_val) ||
        !process_val->IsFunction()) {
         printf("get function fail\n");
    }
    process_fun = Local<Function>::Cast(process_val);
 
    Local<Value> result;
    sstring tmp;

    const int argc = args.Length() -1;
    Local<Value> argv[argc];
    for (int i = 0; i < argc; i++) {
        argv[i] = args[i+1];
    }

    if (!process_fun->Call(context, context->Global(), argc, argv).ToLocal(&result)) {

    } else {

    }
    return make_ready_future<>();
}

// Run JavaScript function
future<> req_service::js_req(args_collection& args, output_stream<char>& out, int tid) {
    v8::Locker locker{isolate};              
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);

    current_tid = tid;
    
    // Switch to V8 context of this tenant
    Local<Context> context = Local<Context>::New(isolate, contexts[current_tid]);
    Context::Scope context_scope(context);
    current_context = &context;

    auto req = make_lw_shared<rqst>(args);
    if (req->args._command_args_count < 1) {
        sstring tmp = to_sstring(msg_syntax_err);
        auto result = reply_builder::build_direct(tmp, tmp.size());
        return out.write(std::move(result));
    }      
    sstring& name = req->args._command_args[0];

    Local<Function> process_fun;
    if (prev_fun_name[current_tid] != name) {
        Local<String> process_name =
            String::NewFromUtf8(isolate, name.c_str(), NewStringType::kNormal)
                .ToLocalChecked();
        Local<Value> process_val;
        if (!context->Global()->Get(context, process_name).ToLocal(&process_val) ||
            !process_val->IsFunction()) {
             printf("get function %s fail\n", name.c_str());
        }
        process_fun = Local<Function>::Cast(process_val);
        prev_fun_name[current_tid] = name;
        prev_fun[current_tid].Reset(isolate, process_fun);
    } else {
         process_fun = Local<Function>::New(isolate, prev_fun[current_tid]);
    }
 
    Local<Value> result;
    sstring tmp;

    const int argc = req->args._command_args_count -1;
    Local<Value> argv[argc];
    for (int i = 0; i < argc; i++) {
	argv[i] = v8::String::NewFromUtf8(isolate, req->args._command_args[i+1].c_str(), v8::NewStringType::kNormal)
          .ToLocalChecked();
        //argv[i] = Number::New(isolate, atoi(req->args._command_args[i+1].c_str()));
    }

    if (!process_fun->Call(context, context->Global(), argc, argv).ToLocal(&result)) {
         auto cstr = "error\n";
         out.write(cstr, strlen(cstr));
    } else {
         if (result->IsArrayBuffer()) {
             // Return raw data
             auto res = Local<ArrayBuffer>::Cast(result);
             auto cont = res->GetContents();
             auto cstr = (char*)cont.Data();
             out.write(cstr, cont.ByteLength());
         } else {
	     // Return data in Redis protocol
             v8::String::Utf8Value str(isolate, result);
             tmp = to_sstring(ToCString(str));
             auto cstr = ToCString(str);
	     auto res = reply_builder::build_direct(tmp, tmp.size());

             out.write(std::move(res));
         }
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

    auto tid = local_req_server().get_tid_direct();

    get_local_sched()->schedule(args, tid);
}

void new_database(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Isolate * isolate = args.GetIsolate();
    HandleScope handle_scope(isolate);

    v8::String::Utf8Value str(isolate, args[0]);
    auto ret = create_db(std::string(*str));
    args.GetReturnValue().Set(v8::Boolean::New(isolate, ret));
}
