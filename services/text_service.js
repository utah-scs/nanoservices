function async_call(service, func, args) {
    return new Promise(function(resolve, reject) {
        Call(service, func,
            function(error, result) {
                if (error) {
                    return reject(error);
                }
                return resolve(result);
            },
            args);
    });
}

function upload_text(val) {
//    print(val);
    let user_mentions = val.match(/@[a-zA-Z0-9-_]+/g);
//    print(user_mentions);
    let urls = val.match(/(http:\/\/|https:\/\/)([a-zA-Z0-9_!~*'().&=+$%-]+)/g);
//    print(urls);
    async_call("user_mention_service.js", "upload_user_mentions", user_mentions)
    .then(
       result => {print(result);}
    );
    async_call("url_shorten_service.js", "upload_urls", urls)
    .then(
       result => {print(result);}
    );
    return "OK\n";
}
