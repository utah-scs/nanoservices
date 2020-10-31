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

function write_home_timeline(req_id, call_id, args) {
    let obj = JSON.parse(args);
    async_call(req_id, call_id, "social_graph_service.js", "get_followers", obj.user_id)
    .then(
       result => {
	       let followers = JSON.parse(result);
	       let mentions = JSON.parse(obj.mentions);
	       let all = [].concat(followers, mentions.userids);
	       let count = 0;
	       let post = new Object();
               post.user_id = obj.user_id;
               post.timestamp = obj.timestamp;
               post.post_id = obj.post_id;

	       let tmp = JSON.stringify(post);

	       for (let i = 0; i < all.length; i++) {
	           let new_args = new Object();
		   new_args.user_id = all[i];
		   new_args.post = tmp;
                   async_call(req_id, call_id, "user_timeline_service.js", "write_user_timeline", JSON.stringify(new_args))
                  .then(
                   result => {
                       count = count + 1;
                       if (count == all.length)
                           Reply(call_id, ServiceName, "ok");
                   });
	       }
                     //      Reply(call_id, ServiceName, "ok");
        }
    );
}
