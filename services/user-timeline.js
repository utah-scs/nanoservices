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

function read(req_id, call_id, args) {
    let obj = JSON.parse(args);
    let tmp = new Object();
    tmp.user_id = obj.parameters.user_id;
    tmp.start = obj.parameters.start;
    tmp.end = obj.parameters.stop;

    async_call(req_id, call_id, "user_timeline_service.js", "read_user_timeline", JSON.stringify(tmp))
    .then(
       result => {
	    let rep = new Object();
            rep._status = 200;
            rep._message = result;
            let ret = JSON.stringify(rep);
            Reply(call_id, ServiceName, ret);
        }
    );
}
