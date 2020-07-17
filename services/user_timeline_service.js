%NeverOptimizeFunction(async_call);
function async_call(req_id, service, func, args) {
    return new Promise(function(resolve, reject) {
        Call(req_id, service, func,
            function(error, result) {
                if (error) {
                    return reject(error);
                }
                return resolve(result);
            },
            args);
    });
}

%NeverOptimizeFunction(write_user_timeline);
function write_user_timeline(req_id, post) {
//    print(post);
    Reply(req_id, ServiceName, "ok");
    return;
}
