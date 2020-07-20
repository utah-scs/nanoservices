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

function gen_random_str(length) {
    let char_map = "abcdefghijklmnopqrstuvwxyzABCDEF"
                    "GHIJKLMNOPQRSTUVWXYZ0123456789";
    let result = "";
    for (let i = 0; i < length; i++) {
        result = result + char_map[Math.floor(Math.random() * (char_map.length))];
    }
    return result;
}

function upload_urls(req_id, arg) {
    let urls = JSON.parse(arg);
    let length = urls.length;
    let results = [];
    for (let i = 0; i < length; i++) {
        let shorturl = "http://short-url/" + gen_random_str(10);
        results.push([urls[i], shorturl]);
    }
    let ret = JSON.stringify(results);
    async_call(req_id, "compose_post_service.js", "upload_post_urls", ret)
    .then(
       result => {Reply(req_id, ServiceName, "ok");}
    );
//    Reply(req_id, ServiceName, ret);
    return results;
}

