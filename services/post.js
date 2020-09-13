function compose(req_id, args) {
	print(args);
    let obj = JSON.parse(args); 
    print(obj);
    print(obj.content);
	            let rep = new Object();
            rep._status = 200;
            rep._message = "OK";
            let ret = JSON.stringify(rep);
            Reply(req_id, ServiceName, ret);
}
