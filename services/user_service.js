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

function user_register(req_id, userid, username) {
    DBSet("user_mention_service.js", username, str2ab(userid));
}

function upload_creator_with_userid(req_id, user_id, username) {
    let creator = new Object();
    creator.username = username;
    creator.user_id = user_id;
    let arg = JSON.stringify(creator);
    async_call(req_id, "compose_post_service.js", "upload_creator", arg);
    Reply(req_id, ServiceName, "ok");
}
