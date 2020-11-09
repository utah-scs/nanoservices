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
  return String.fromCharCode.apply(null, new Uint16Array(buf)).substring(2);
}

function str2ab(str) {
  var buf = new ArrayBuffer(str.length * 2); // 2 bytes for each char
  var bufView = new Uint16Array(buf);
  for (var i = 0, strLen = str.length; i < strLen; i++) {
    bufView[i] = str.charCodeAt(i);
  }
  return buf;
}

function abversion(ab) {
  let v = new DataView(ab);
  return v.getUint32(0, true);
}

function follow(req_id, call_id, args) {
    let obj = JSON.parse(args);
    let user_id = obj.username;
    let followee_id = obj.followee_name;
    let version;
    let abort = "abort";
    while (abort == "abort") {
        let update = [];
        let followees = DBGet("social_graph_service.js", user_id);
        if (followees.byteLength > 0) {
            update = JSON.parse(ab2str(followees));
            version = abversion(followees);
        } else
            version = 0;
 
        update.push(followee_id);
        abort = DBSet("social_graph_service.js", 
                user_id, str2ab(JSON.stringify(update)), version);
    }

    abort = "abort";
    while (abort == "abort") {
        let followers = [];
        let tmp = DBGet("social_graph_service.js", followee_id + "followers");
        if (tmp.byteLength != 0) {
            followers = JSON.parse(ab2str(tmp));
            version = abversion(tmp);
        } else
            version = 0;
 
        followers.push(user_id);
        abort = DBSet("social_graph_service.js", 
                followee_id + "followers", str2ab(JSON.stringify(followers)), version);
    }

    Reply(call_id, ServiceName, "ok");
}

function get_followers(req_id, call_id, user_id) {
    let followers;
    let tmp = DBGet("social_graph_service.js", user_id + "followers");
    if (tmp.byteLength != 0)
        followers = ab2str(tmp);
    else
	followers = "[]";
    Reply(call_id, ServiceName, followers);
}
