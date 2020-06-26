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

function upload_urls(req_id, urls) {
    let length = urls.length;
    let results = [];
    for (let i = 0; i < length; i++) {
        let shorturl = "http://short-url/" + gen_random_str(10);
        results.push([urls[i], shorturl]);
    }
//    let ret = JSON.stringify(result);
//    print(ret);
    Reply(req_id, ServiceName, "OK");
    return results;
}

