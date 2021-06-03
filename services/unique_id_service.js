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

let counter = 0;
let current_timestamp = 0;

function get_counter(t) {
    if (current_timestamp > t) {
        print("Timestamps are not incremental.");
    }
    if (current_timestamp == t) {
        return counter++;
    } else {
        current_timestamp = t;
	counter = 0;
	return counter++;
    }
}

function upload_unique_id(req_id, call_id, post_type) {
    let t = Date.now();
    let idx = get_counter(t);
    let post_id = CoreID.toString() + t.toString() + idx.toString(); 

    let params = new Object();
    params.post_id = post_id;
    params.post_type = post_type;

    async_call(req_id, call_id, "compose_post_service.js", "upload_unique_id", JSON.stringify(params))
    .then(
       result => {
           Reply(req_id, call_id, ServiceName, "ok");
       }
    );
}
