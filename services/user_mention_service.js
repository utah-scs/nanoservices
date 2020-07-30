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

function upload_user_mentions(req_id, arg) {
    let mentions = JSON.parse(arg);
    let length = mentions.length;
    let results = new Object();
    results.usernames = [];
    results.userids = [];
    for (let i = 0; i < length; i++) {
        let username = mentions[i].substring(1);
        let userid = ab2str(DBGet("user_mention_service.js", username));
        results.usernames.push(username);
        results.userids.push(userid);
    }
    let ret = JSON.stringify(results);
    async_call(req_id, "compose_post_service.js", "upload_user_mentions", ret);
    Reply(req_id, ServiceName, "ok");
}
