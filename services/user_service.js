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

function user_register(req_id, call_id, args) {
    let obj = JSON.parse(args);
    let userid = obj.user_id;
    let username = obj.username;
    DBSet("user_mention_service.js", username, str2ab(userid));
    Reply(call_id, ServiceName, "ok");
}

function upload_creator_with_userid(req_id, call_id, user) {
    let obj = JSON.parse(user);
    let creator = new Object();
    creator.username = obj.username;
    creator.user_id = obj.user_id;
    let arg = JSON.stringify(creator);
    async_call(req_id, call_id, "compose_post_service.js", "upload_creator", arg)
    .then(
        result => {
            Reply(call_id, ServiceName, "ok");
	});
}
