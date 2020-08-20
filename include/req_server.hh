#pragma once
#include "include/seastarkv.hh"
#include "include/reply_builder.hh"
#include <unordered_map>
#include <iterator>

using namespace std;
using namespace redis;
using namespace seastar;
using namespace v8;

class req_service;
extern distributed<req_service> req_server;
inline distributed<req_service>& get_req_server() {
    return req_server;
}
inline req_service& local_req_server() {
    return req_server.local();
}

inline unsigned get_cpu(const sstring& key) {
    return std::hash<sstring>()(key) % smp::count;
}
inline unsigned get_cpu(const redis_key& key) {
    return key.hash() % smp::count;
}

namespace shredder {
void db_get(const v8::FunctionCallbackInfo<v8::Value>& args);
void db_set(const v8::FunctionCallbackInfo<v8::Value>& args);
void db_del(const v8::FunctionCallbackInfo<v8::Value>& args);
void js_print(const v8::FunctionCallbackInfo<v8::Value>& args);
void init_iterator(const v8::FunctionCallbackInfo<v8::Value>& args);
void iterator_next(const v8::FunctionCallbackInfo<v8::Value>& args);
void get_hash_table(const v8::FunctionCallbackInfo<v8::Value>& args);
void load_fb_graph(const v8::FunctionCallbackInfo<v8::Value>& args);
void call_function(const v8::FunctionCallbackInfo<v8::Value>& args);
void new_database(const v8::FunctionCallbackInfo<v8::Value>& args);
void reply(const v8::FunctionCallbackInfo<v8::Value>& args);
}

using message = scattered_message<char>;
class req_service {
private:
    // The tenant id tied to the currently running script.
    // The JavaScript context of the currently running script.
    Local<Context>* current_context;
    Global<Context> contexts[NUM_SERVICES];
    unsigned unallocated_ctx = 0;
    unordered_map<std::string, unsigned> ctx_map;
    char** argv;
    Isolate::CreateParams create_params;
    Global<ObjectTemplate> args_templ;

    // Semaphore to signal incoming requests to JS thread.
    semaphore sem{0};

    // The stucture to store an incoming request.
    struct rqst {
        args_collection& args;
        sstring key;
        sstring val;
        int tenant_id;
        bool put;
        rqst(args_collection& a)
            :args(a)
        {}
        ~rqst(){}
    };
   
    MaybeLocal<String> read_file(Isolate* isolate, const string& name) {
        FILE* file = fopen(name.c_str(), "rb");
        if (file == NULL) return MaybeLocal<String>();
   
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        rewind(file);
   
        char* chars = new char[size + 1];
        chars[size] = '\0';
        for (size_t i = 0; i < size;) {
            i += fread(&chars[i], 1, size - i, file);
            if (ferror(file)) {
                fclose(file);
                return MaybeLocal<String>();
            }
        }
        fclose(file);
        MaybeLocal<String> result = String::NewFromUtf8(
            isolate, chars, NewStringType::kNormal, static_cast<int>(size));
        delete[] chars;
        return result;
    }

public:
    Isolate* isolate;

    req_service(char** a)
        : argv(a)
    {
        create_params.array_buffer_allocator =
            v8::ArrayBuffer::Allocator::NewDefaultAllocator();
        isolate = Isolate::New(create_params);
    }

    future<> start();
    future<> stop();
    future<> register_service(std::string service);
    future<> js();
    future<> js_req(std::string req_id, sstring service, sstring function, std::string args, output_stream<char>& out);
    future<> run_func(std::string req_id, std::string service, std::string function, std::string jsargs);
    future<> run_callback(std::string req_id, std::string service, sstring ret);
};
