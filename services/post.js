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

function compose(req_id, args) {
    let obj = JSON.parse(args); 
    let body = obj.content;

    let match;
    let tmp = body;
    match = /&/.exec(tmp);
    let username = tmp.substring(9, match.index);
    tmp = tmp.substring(match.index + 1);
    match = /&/.exec(tmp);
    let userid = tmp.substring(8, match.index);
    tmp = tmp.substring(match.index + 1);
    match = /&/.exec(tmp);
    let text = tmp.substring(5, match.index);
    tmp = tmp.substring(match.index + 1);
    match = /&/.exec(tmp);
    let media_ids = JSON.parse(tmp.substring(10, match.index));
    tmp = tmp.substring(match.index + 1);
    match = /&/.exec(tmp);
    let media_types = JSON.parse(tmp.substring(12, match.index));
    tmp = tmp.substring(match.index + 1);
    let post_type = tmp.substring(10);

    return async_call(req_id, "text_service.js", "upload_text", text)
    //return async_call(req_id, "user.js", "login", args)
    .then(
       result => {
            let rep = new Object();
            rep._status = 200;
            rep._message = result;
            let ret = JSON.stringify(rep);
            Reply(req_id, ServiceName, ret);}
    );
}