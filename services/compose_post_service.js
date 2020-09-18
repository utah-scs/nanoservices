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
function upload_post(req_id) {
}

function upload_urls(req_id, urls) {
//    print("upload urls");
    let key = req_id + "urls";
    DBSet("compose_post_service.js", key, str2ab(urls));
    Reply(req_id, ServiceName, "ok");
}

function upload_media(req_id, media) {
//    print("upload media");
    let key = req_id + "media";
    DBSet("compose_post_service.js", key, str2ab(media));
    Reply(req_id, ServiceName, "ok");
}

function upload_user_mentions(req_id, mentions) {
//    print("upload user mentions");
    let key = req_id + "mentions";
    DBSet("compose_post_service.js", key, str2ab(mentions));
    Reply(req_id, ServiceName, "ok");
}

function upload_creator(req_id, creator) {
//    print("upload creator");
    let key = req_id + "creator";
    DBSet("compose_post_service.js", key, str2ab(creator));
    Reply(req_id, ServiceName, "ok");
}

function upload_unique_id(req_id, args) {
//    print("upload unique_id");
    let obj = JSON.parse(args);
    let key1 = req_id + "post_id";
    DBSet("compose_post_service.js", key1, str2ab(obj.post_id));
    let key2 = req_id + "post_type";
    DBSet("compose_post_service.js", key2, str2ab(obj.post_type));

    Reply(req_id, ServiceName, "ok");
}


