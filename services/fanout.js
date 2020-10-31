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
    let num = obj.headers.NUM;
    let endtime = new Date().getTime() + Number(ms);
//    while (new Date().getTime() < endtime) {
  //  }

    let count = 0;
    for (let i = 0; i < num; i++) {
        async_call(req_id, call_id, "microbenchmark4.js", "func", args)
        .then(
           result => {
                count = count + 1;
                if (count == num) {
                    let rep = new Object();
                    rep._status = 200;
                    rep._message = "OK";
                    let ret = JSON.stringify(rep);
                    Reply(call_id, ServiceName, ret);
                }
           }
        );
    }
}
