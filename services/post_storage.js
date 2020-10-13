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

function store_post(req_id, args) {
	//print("store post");
    let post_id = JSON.parse(args).post_id;
    DBSet("post_storage.js", post_id, str2ab(args));
    Reply(req_id, ServiceName, "ok");
}

function read_post(req_id, post_id) {
    let post = ab2str(DBGet("post_storage.js", post_id));
    Reply(req_id, ServiceName, post);
}

function read_posts(req_id, args) {
    let arr = JSON.parse(args);
    let ret = [];
    let id;
    for (id in arr) {
        let post = ab2str(DBGet("post_storage.js", id));
	ret.push(post);
    }
    Reply(req_id, ServiceName, JSON.stringify(ret));
}
