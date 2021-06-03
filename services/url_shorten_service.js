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

function gen_random_str(length) {
    let char_map = "abcdefghijklmnopqrstuvwxyzABCDEF"
                    "GHIJKLMNOPQRSTUVWXYZ0123456789";
    let result = "";
    for (let i = 0; i < length; i++) {
        result = result + char_map[Math.floor(Math.random() * (char_map.length))];
    }
    return result;
}

function upload_urls(req_id, call_id, arg) {
    let urls = JSON.parse(arg);
    let length = urls.length;
    let results = new Object();
    results.urls = [];
    results.shorturls = [];
    for (let i = 0; i < length; i++) {
        let shorturl = "http://short-url/" + gen_random_str(10);
        results.urls.push(urls[i]);
        results.shorturls.push(shorturl);
    }
    let ret = JSON.stringify(results);
    async_call(req_id, call_id, "compose_post_service.js", "upload_urls", ret);
    Reply(req_id, call_id, ServiceName, ret);
}

