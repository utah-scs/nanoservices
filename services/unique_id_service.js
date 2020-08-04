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

let current_timestamp = -1;
let counter = 0;

function get_counter(timestamp) {
  if (current_timestamp > timestamp) {
    print("error");
    return;
  }
  if (current_timestamp == timestamp) {
    return counter++;
  } else {
    current_timestamp = timestamp;
    counter = 0;
    return counter++;
  }
}

function upload_unique_id(req_id, type) {
  let timestamp = Date.now();
  let idx = get_counter(timestamp);
  let post_id = CPUID.toString() + timestamp.toString() + idx.toString();
  let obj = new Object();
  obj.post_id = post_id;
  obj.post_type = type;
  let args = JSON.stringify(obj);
  async_call(req_id, "compose_post_service.js", "upload_unique_id", args);
  Reply(req_id, ServiceName, "ok");
}
