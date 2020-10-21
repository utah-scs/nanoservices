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

function compose(req_id, call_id, args) {
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

    let count = 0;

    let media_args = new Object();
    media_args.ids = media_ids;
    media_args.types = media_types;

    async_call(req_id, "unique_id_service.js", "upload_unique_id", post_type)
    .then(
       result => {
	    count = count + 1;
	    if (count == 4) {
                let rep = new Object();
                rep._status = 200;
                rep._message = result;
                let ret = JSON.stringify(rep);
                Reply(call_id, ServiceName, ret);
	    }
        }
    );

    let user = new Object();
    user.user_id = userid;
    user.username = username;
    
    async_call(req_id, "user_service.js", "upload_creator_with_userid", JSON.stringify(user))
    .then(
       result => {
	    count = count + 1;
	    if (count == 4) {
                let rep = new Object();
                rep._status = 200;
                rep._message = result;
                let ret = JSON.stringify(rep);
                Reply(call_id, ServiceName, ret);
	    }
        }
    );
	
    async_call(req_id, "media_service.js", "upload_media", JSON.stringify(media_args))
    .then(
       result => {
	    count = count + 1;
	    if (count == 4) {
                let rep = new Object();
                rep._status = 200;
                rep._message = result;
                let ret = JSON.stringify(rep);
                Reply(call_id, ServiceName, ret);
	    }
        }
    );
	
    return async_call(req_id, "text_service.js", "upload_text", text)
    .then(
       result => {
	    count = count + 1;
	    if (count == 4) {
                let rep = new Object();
                rep._status = 200;
                rep._message = result;
                let ret = JSON.stringify(rep);
                Reply(call_id, ServiceName, ret);
	    }
        }
    );
}
