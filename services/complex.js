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

function l0(req_id, call_id, args) {
//    let obj = JSON.parse(args);
//    let ms = obj.headers.MS;
//    let num = obj.headers.NUM;
//    let endtime = new Date().getTime() + Number(ms);
    let endtime = new Date().getTime() + 10;
    while (new Date().getTime() < endtime) {
    }

    let count = 0;
    let num = 2;
    async_call(req_id, call_id, "complex.js", "l1", args).then(
	result => {
                count = count + 1;
                if (count == num + 1) {
                    let rep = new Object();
                    rep._status = 200;
                    rep._message = "OK";
                    let ret = JSON.stringify(rep);
                    Reply(req_id, call_id, ServiceName, ret);
                }
           }
        );

    for (let i = 0; i < num; i++) {
        async_call(req_id, call_id, "complex.js", "leaf", args)
        .then(
           result => {
                count = count + 1;
                if (count == num + 1) {
                    let rep = new Object();
                    rep._status = 200;
                    rep._message = "OK";
                    let ret = JSON.stringify(rep);
                    Reply(req_id, call_id, ServiceName, ret);
                }
           }
        );
    }
}

function l1(req_id, call_id, args) {
//    let obj = JSON.parse(args);
//    let ms = obj.headers.MS;
//    let num = obj.headers.NUM;
//    let endtime = new Date().getTime() + Number(ms);
    let endtime = new Date().getTime() + 10;
    while (new Date().getTime() < endtime) {
    }

    let count = 0;
    let num = 2;
    async_call(req_id, call_id, "complex.js", "l2", args).then(
        result => {
                count = count + 1;
                if (count == num + 1) {
                    let rep = new Object();
                    rep._status = 200;
                    rep._message = "OK";
                    let ret = JSON.stringify(rep);
                    Reply(req_id, call_id, ServiceName, ret);
                }
           }
        );

    for (let i = 0; i < num; i++) {
        async_call(req_id, call_id, "complex.js", "leaf", args)
        .then(
           result => {
                count = count + 1;
                if (count == num + 1) {
                    let rep = new Object();
                    rep._status = 200;
                    rep._message = "OK";
                    let ret = JSON.stringify(rep);
                    Reply(req_id, call_id, ServiceName, ret);
                }
           }
        );
    }
}

function l2(req_id, call_id, args) {
//    let obj = JSON.parse(args);
//    let ms = obj.headers.MS;
//    let num = obj.headers.NUM;
//    let endtime = new Date().getTime() + Number(ms);
    let endtime = new Date().getTime() + 10;
    while (new Date().getTime() < endtime) {
    }

    let count = 0;
    let num = 3;
    for (let i = 0; i < num; i++) {
        async_call(req_id, call_id, "complex.js", "leaf", args)
        .then(
           result => {
                count = count + 1;
                if (count == num) {
                    let rep = new Object();
                    rep._status = 200;
                    rep._message = "OK";
                    let ret = JSON.stringify(rep);
                    Reply(req_id, call_id, ServiceName, ret);
                }
           }
        );
    }
}

function leaf(req_id, call_id, args) {
    let endtime = new Date().getTime() + 10;
    while (new Date().getTime() < endtime) {
    }
    let rep = new Object();
    rep._status = 200;
    rep._message = "OK";
    let ret = JSON.stringify(rep);
    Reply(req_id, call_id, ServiceName, ret);
}
