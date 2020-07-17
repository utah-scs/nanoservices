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

%NeverOptimizeFunction(upload_post_urls);
function upload_post_urls(req_id, urls) {
    async_call(req_id, "user_timeline_service.js", "write_user_timeline", urls)
    .then(
       result => {Reply(req_id, ServiceName, "ok");}
    );
//Reply(req_id, ServiceName, "ok");
    return "OK\n";
}
