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

function ab2str(buf) {
  return String.fromCharCode.apply(null, new Uint16Array(buf));
}

function str2ab(str) {
  var buf = new ArrayBuffer(str.length * 2); // 2 bytes for each char
  var bufView = new Uint16Array(buf);
  for (var i = 0, strLen = str.length; i < strLen; i++) {
    bufView[i] = str.charCodeAt(i);
  }
  return buf;
}

function follow(req_id, args) {
    let obj = JSON.parse(args);
    let user_id = obj.username;
    let followee_id = obj.followee_name;
    let followees = DBGet("social_graph_service.js", user_id);
    let update = [];
    if (followees.byteLength != 0) {
	update = JSON.parse(ab2str(followees));
    }
    update.push(followee_id);
    DBSet("social_graph_service.js", user_id, str2ab(JSON.stringify(update)));
    Reply(req_id, ServiceName, "ok");
}