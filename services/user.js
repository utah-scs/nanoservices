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

function init_db(req_id, args) {
    let user = new Object();
    user.id = "57a98d98e4b00679b4a830af";
    user.firstname = "Eve";
    user.lastName = "Berger";
    user.username = "user";
    user.salt = "c748112bc027878aa62812ba1ae00e40ad46d497";
    user.password = Sha1("password" + user.salt);
    
    DBSet("user.js", "user", str2ab(JSON.stringify(user)));
    let rep = new Object();
    rep._status = 200;
    rep._message = "ok";
    let ret = JSON.stringify(rep);
    Reply(req_id, ServiceName, ret);
}

function login(req_id, args) {
    let obj = JSON.parse(args);
    let str = Base64Decode(obj.headers.Authorization.substr(6));
    let auth = str.split(":");
    let username = auth[0];
    let password = auth[1];
    let user = JSON.parse(ab2str(DBGet("user.js", username)));
    let pw = Sha1(password + user.salt);
    let rep = new Object();
    if (pw != user.password) {
        rep._status = 401;
        rep._message = "Authorization failed.";
    } else {
        rep._status = 200;
        rep._message = "ok";
    }
    let ret = JSON.stringify(rep);
    Reply(req_id, ServiceName, ret);
}

function mongo_login(req_id, args) {
    let obj = JSON.parse(args);
    let str = Base64Decode(obj.headers.Authorization.substr(6));
    let auth = str.split(":");
    let username = auth[0];
    let password = auth[1];

    let user = JSON.parse(MongoGet("users", "customers", "user"));
    let salt = user.salt;
    let pw = Sha1(password + salt);
    let rep = new Object();
    if (pw != user.password) {
        rep._status = 401;
        rep._message = "Authorization failed.";
    } else {
        rep._status = 200;
        rep._message = "ok";
    }
    let ret = JSON.stringify(rep);
    Reply(req_id, ServiceName, ret);
}

function register(req_id, args) {
    let obj = JSON.parse(args);
    let body = obj.content;

    let match;
    let tmp = body;
    match = /&/.exec(tmp);
    let first_name = tmp.substring(11, match.index);
    tmp = tmp.substring(match.index + 1);
    match = /&/.exec(tmp);
    let last_name = tmp.substring(10, match.index);
    tmp = tmp.substring(match.index + 1);
    match = /&/.exec(tmp);
    let username = tmp.substring(9, match.index);
    tmp = tmp.substring(match.index + 1);
    match = /&/.exec(tmp);
    let password = tmp.substring(9, match.index);
    tmp = tmp.substring(match.index + 1);
    let user_id = tmp.substring(8);

    let params = new Object();
    params.user_id = user_id;
    params.username = username;
    return async_call(req_id, "user_service.js", "user_register", JSON.stringify(params))
    .then(
       result => {
            let rep = new Object();
            rep._status = 200;
            rep._message = "OK";
            let ret = JSON.stringify(rep);
            Reply(req_id, ServiceName, ret);}
    );
}

function follow(req_id, args) {
    let obj = JSON.parse(args);
    let body = obj.content;

    let match;
    let tmp = body;
    match = /&/.exec(tmp);
    let username = tmp.substring(10, match.index);
    tmp = tmp.substring(match.index + 1);
    match = /&/.exec(tmp);
    let followee_name = tmp.substring(14);

    let params = new Object();
    params.username = username;
    params.followee_name = followee_name;
    return async_call(req_id, "social_graph_service.js", "follow", JSON.stringify(params))
    .then(
       result => {
            let rep = new Object();
            rep._status = 200;
            rep._message = "OK";
            let ret = JSON.stringify(rep);
            Reply(req_id, ServiceName, ret);}
    );
}
