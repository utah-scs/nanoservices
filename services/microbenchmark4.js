function func(req_id, args) {
    let obj = JSON.parse(args);
    let ms = obj.headers.MS;
    let endtime = new Date().getTime() + Number(ms);
    while (new Date().getTime() < endtime) {
    }
    let rep = new Object();
    rep._status = 200;
    rep._message = "ok";
    let ret = JSON.stringify(rep);
    Reply(req_id, ServiceName, ret);
}
