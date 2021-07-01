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

function run_10ms(req_id, call_id, args) {
    let endtime = new Date().getTime() + 10;
    while (new Date().getTime() < endtime) {
    }

    let rep = new Object();
    rep._status = 200;
    rep._message = "OK";
    let ret = JSON.stringify(rep);
    Reply(req_id, call_id, ServiceName, ret);
}

function run_100ms(req_id, call_id, args) {
    let endtime = new Date().getTime() + 100;
    while (new Date().getTime() < endtime) {
    }

    let rep = new Object();
    rep._status = 200;
    rep._message = "OK";
    let ret = JSON.stringify(rep);
    Reply(req_id, call_id, ServiceName, ret);
}
