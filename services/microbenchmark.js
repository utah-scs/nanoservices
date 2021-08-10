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

function func(req_id, call_id, args) {
    let obj = JSON.parse(args);
    let ms = obj.headers.MS;
    let endtime = new Date().getTime() + Number(ms);
    while (new Date().getTime() < endtime) {
    }
    return async_call(req_id, call_id, "microbenchmark2.js", "func", args)
    .then(
       result => {
            let rep = new Object();
            rep._status = 200;
            rep._message = "OK";
            let ret = JSON.stringify(rep);
            Reply(req_id, call_id, ServiceName, ret);}
    );
}

function chain(req_id, call_id, args) {
    let obj = JSON.parse(args);
    let ms = obj.headers.MS;
    let l = obj.headers.L - 1;
    let endtime = new Date().getTime() + Number(ms);
    while (new Date().getTime() < endtime) {
    }
    if (l == 0) {
        let rep = new Object();
        rep._status = 200;
        rep._message = "OK";
        let ret = JSON.stringify(rep);
        Reply(req_id, call_id, ServiceName, ret);
	return;
    }

    obj.headers.L = l;
    return async_call(req_id, call_id, "microbenchmark.js", "chain", JSON.stringify(obj))
    .then(
       result => {
            let rep = new Object();
            rep._status = 200;
            rep._message = "OK";
            let ret = JSON.stringify(rep);
            Reply(req_id, call_id, ServiceName, ret);}
    );
}
