function async_call(req_id, call_id, service, func, args) {
    return new Promise(function(resolve, reject) {
        Call(req_id, call_id, service, func,
            function(error, result) {
                if (error) {
                    return reject(error);
                }
                return resolve(result);
            },
            args);
    });
}

function upload_media(req_id, call_id, args) {
    async_call(req_id, call_id, "compose_post_service.js", "upload_media", args)
    .then(
       result => {Reply(req_id, call_id, ServiceName, "ok");}
    );
}
