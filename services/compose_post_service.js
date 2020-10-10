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
    let post = new Object();
    post.req_id = req_id;
    post.text = ab2str(DBGet("compose_post_service.js", req_id + "text"));
    post.post_id = ab2str(DBGet("compose_post_service.js", req_id + "post_id"));
    post.timestamp = new Date().getTime();
    post.post_type = ab2str(DBGet("compose_post_service.js", req_id + "post_type"));
    post.creator = ab2str(DBGet("compose_post_service.js", req_id + "creator"));
    post.user_mentions = ab2str(DBGet("compose_post_service.js", req_id + "mentions"));
    post.media = ab2str(DBGet("compose_post_service.js", req_id + "media"));
    post.urls = ab2str(DBGet("compose_post_service.js", req_id + "urls"));

    let count = 0;
    async_call(req_id, "post_storage.js", "store_post", JSON.stringify(post))
    .then(
       result => {
            count = count + 1;
            if (count == 3) {
                let rep = new Object();
                rep._status = 200;
                rep._message = result;
                let ret = JSON.stringify(rep);
                Reply(req_id, ServiceName, ret);
            }
        }
    );
 
    let user_id = JSON.parse(post.creator).user_id;
    let args1 = new Object();
    args1.user_id = user_id;
    args1.timestamp = post.timestamp;
    args1.post_id = post.post_id;
    async_call(req_id, "user_timeline_service.js", "write_user_timeline", JSON.stringify(args1))
    .then(
       result => {
            count = count + 1;
            if (count == 3) {
                let rep = new Object();
                rep._status = 200;
                rep._message = result;
                let ret = JSON.stringify(rep);
                Reply(req_id, ServiceName, ret);
            }
        }
    );

    let args2 = new Object();
    args2.user_id = user_id;
    args2.timestamp = post.timestamp;
    args2.post_id = post.post_id;
    args2.mentions = post.user_mentions;

    async_call(req_id, "home_timeline_service.js", "upload_home_timeline", JSON.stringify(args2))
    .then(
       result => {
            count = count + 1;
            if (count == 3) {
                let rep = new Object();
                rep._status = 200;
                rep._message = result;
                let ret = JSON.stringify(rep);
                Reply(req_id, ServiceName, ret);
            }
        }
    );
}

function check_upload_post(req_id) {
    let key = req_id + "count";
    let count = DBGet("compose_post_service.js", key);
    let new_count = 0;
    if (count.byteLength == 0) {
	new_count = 1;
        DBSet("compose_post_service.js", key, str2ab(new_count.toString()));
        Reply(req_id, ServiceName, "ok");
    } else {
        new_count = Number(ab2str(count)) + 1;
	if (new_count == 6) {
            upload_post(req_id);
	} else {
            DBSet("compose_post_service.js", key, str2ab(new_count.toString()));
            Reply(req_id, ServiceName, "ok");
	}
    }
}

function upload_urls(req_id, urls) {
    let key = req_id + "urls";
    DBSet("compose_post_service.js", key, str2ab(urls));
    check_upload_post(req_id);	
}

function upload_media(req_id, media) {
    let key = req_id + "media";
    DBSet("compose_post_service.js", key, str2ab(media));
    check_upload_post(req_id);	
}

function upload_user_mentions(req_id, mentions) {
    let key = req_id + "mentions";
    DBSet("compose_post_service.js", key, str2ab(mentions));
    check_upload_post(req_id);	
}

function upload_creator(req_id, creator) {
    let key = req_id + "creator";
    DBSet("compose_post_service.js", key, str2ab(creator));
    check_upload_post(req_id);	
}

function upload_text(req_id, text) {
    let key = req_id + "text";
    DBSet("compose_post_service.js", key, str2ab(text));
    check_upload_post(req_id);	
//    upload_post(req_id);
}

function upload_unique_id(req_id, args) {
    let obj = JSON.parse(args);
    let key1 = req_id + "post_id";
    DBSet("compose_post_service.js", key1, str2ab(obj.post_id));
    let key2 = req_id + "post_type";
    DBSet("compose_post_service.js", key2, str2ab(obj.post_type));

    check_upload_post(req_id);	
}


