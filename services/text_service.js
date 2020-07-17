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

%NeverOptimizeFunction(upload_text);
function upload_text(req_id, val) {
    let user_mentions = val.match(/@[a-zA-Z0-9-_]+/g);
    let urls = val.match(/(http:\/\/|https:\/\/)([a-zA-Z0-9_!~*'().&=+$%-]+)/g);
//    async_call(req_id, "user_mention_service.js", "upload_user_mentions", user_mentions)
//    .then(
//       result => {print(result);/*Reply(req_id, ServiceName, result);*/}
//    );
    let arg = JSON.stringify(urls);
    async_call(req_id, "url_shorten_service.js", "upload_urls", arg)
    .then(
       result => {Reply(req_id, ServiceName, "ok");}
    );

    return "OK\n";
}
