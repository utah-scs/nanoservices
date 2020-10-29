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

function store_post(req_id, call_id, args) {
    let post_id = JSON.parse(args).post_id;
    DBSet("post_storage.js", post_id, str2ab(args));
    Reply(call_id, ServiceName, "ok");
}

function read_post(req_id, call_id, post_id) {
    let post = ab2str(DBGet("post_storage.js", post_id));
    Reply(call_id, ServiceName, post);
}

function read_posts(req_id, call_id, args) {
    let arr = JSON.parse(args);
    let ret = [];
    for (let i = 0; i < arr.length; i++) {
        let post = JSON.parse(ab2str(DBGet("post_storage.js", arr[i])));
	ret.push(post);
    }
    Reply(call_id, ServiceName, JSON.stringify(ret));
}
