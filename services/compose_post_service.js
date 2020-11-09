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

function upload_post(req_id, call_id) {
    let post = new Object();
    post.text = ab2str(DBGet("compose_post_service.js", req_id + "text"));
    post.post_id = ab2str(DBGet("compose_post_service.js", req_id + "post_id"));
    post.timestamp = new Date().getTime();
    post.post_type = ab2str(DBGet("compose_post_service.js", req_id + "post_type"));
    post.creator = ab2str(DBGet("compose_post_service.js", req_id + "creator"));
    let user_mentions = ab2str(DBGet("compose_post_service.js", req_id + "mentions"));
    post.user_mentions = user_mentions;
    post.media = ab2str(DBGet("compose_post_service.js", req_id + "media"));
    post.urls = ab2str(DBGet("compose_post_service.js", req_id + "urls"));

    let count = 0;
    async_call(req_id, call_id, "post_storage.js", "store_post", JSON.stringify(post))
    .then(
       result => {
            count = count + 1;
            if (count == 3) {
                let rep = new Object();
                rep._status = 200;
                rep._message = result;
                let ret = JSON.stringify(rep);
                Reply(call_id, ServiceName, ret);
            }
        }
    );
 
    let user_id = JSON.parse(post.creator).user_id;
    let tmp = new Object();
    tmp.user_id = user_id;
    tmp.timestamp = post.timestamp;
    tmp.post_id = post.post_id;
    let args1 = new Object();
    args1.user_id = user_id;
    args1.post = JSON.stringify(tmp);
    async_call(req_id, call_id, "user_timeline_service.js", "write_user_timeline", JSON.stringify(args1))
    .then(
       result => {
            count = count + 1;
            if (count == 3) {
                let rep = new Object();
                rep._status = 200;
                rep._message = result;
                let ret = JSON.stringify(rep);
                Reply(call_id, ServiceName, ret);
            }
        }
    );

    let args2 = new Object();
    args2.user_id = user_id;
    args2.timestamp = post.timestamp;
    args2.post_id = post.post_id;
    args2.mentions = user_mentions;

    async_call(req_id, call_id, "write_home_timeline_service.js", "write_home_timeline", JSON.stringify(args2))
    .then(
       result => {
            count = count + 1;
            if (count == 3) {
                let rep = new Object();
                rep._status = 200;
                rep._message = result;
                let ret = JSON.stringify(rep);
                Reply(call_id, ServiceName, ret);
            }
        }
    );
}

function check_upload_post(req_id, call_id) {
    let key = req_id + "count";
    let abort = "abort";
    while (abort == "abort") {
        let count = DBGet("compose_post_service.js", key);
        let new_count = 0;
        if (count.byteLength == 0) {
            new_count = 1;
            abort = DBSet("compose_post_service.js", key, str2ab(new_count.toString()), 0);
        } else {
            let version = abversion(count);
            new_count = Number(ab2str(count)) + 1;
            if (new_count == 6) {
                upload_post(req_id, call_id);
                return;
            } else {
                abort = DBSet("compose_post_service.js", key, str2ab(new_count.toString()), version);
            }
        }
    }
    Reply(call_id, ServiceName, "ok");
}

function upload_urls(req_id, call_id, urls) {
    let key = req_id + "urls";
    DBSet("compose_post_service.js", key, str2ab(urls), 0);
    check_upload_post(req_id, call_id);	
}

function upload_media(req_id, call_id, media) {
    let key = req_id + "media";
    DBSet("compose_post_service.js", key, str2ab(media), 0);
    check_upload_post(req_id, call_id);	
}

function upload_user_mentions(req_id, call_id, mentions) {
    let key = req_id + "mentions";
    DBSet("compose_post_service.js", key, str2ab(mentions), 0);
    check_upload_post(req_id, call_id);	
}

function upload_creator(req_id, call_id, creator) {
    let key = req_id + "creator";
    DBSet("compose_post_service.js", key, str2ab(creator), 0);
    check_upload_post(req_id, call_id);	
}

function upload_text(req_id, call_id, text) {
    let key = req_id + "text";
    DBSet("compose_post_service.js", key, str2ab(text), 0);
    check_upload_post(req_id, call_id);	
}

function upload_unique_id(req_id, call_id, args) {
    let obj = JSON.parse(args);
    let key1 = req_id + "post_id";
    DBSet("compose_post_service.js", key1, str2ab(obj.post_id), 0);
    let key2 = req_id + "post_type";
    DBSet("compose_post_service.js", key2, str2ab(obj.post_type), 0);

    check_upload_post(req_id, call_id);	
}


