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

Array.prototype.push_sorted = function(e, compare) {
  this.splice((function(arr) {
    let m = 0;
    let n = arr.length - 1;

    let k;
    while(m <= n) {
      k = (n + m) >> 1;
      let cmp = compare(e, arr[k]);

      if(cmp > 0) m = k + 1;
        else if(cmp < 0) n = k - 1;
        else return k;
    }

    return m;
  })(this), 0, e);

  return this.length;
};

const comp = (a, b) => a.timestamp > b.timestamp;

function write_user_timeline(req_id, call_id, args) {
    let e = JSON.parse(args);
    let post = JSON.parse(e.post);
    let abort = "abort";
    while (abort == "abort") {
        let posts = [];
        let tmp = DBGet("user_timeline_service.js", e.user_id);
        let version;
        if (tmp.byteLength != 0) {
            posts = JSON.parse(ab2str(tmp));
            version = abversion(tmp);
        } else
            version = 0;
        
        posts.push_sorted(post, comp);
        abort = DBSet("user_timeline_service.js", e.user_id, str2ab(JSON.stringify(posts)), version);
    }

    Reply(req_id, call_id, ServiceName, "ok");
    return;
}

function read_user_timeline(req_id, call_id, args) {
    let req = JSON.parse(args);
    let posts = [];
    let arr = [];
    let tmp = DBGet("user_timeline_service.js", req.user_id);
    if (tmp.byteLength != 0)
        arr = JSON.parse(ab2str(tmp));
    let range = arr.slice(Math.min(req.start, arr.length), Math.min(req.end, arr.length)+1);
    let e;
    for (let i = 0; i < range.length; i++) {
	posts.push(arr[i].post_id);
    }
    
    async_call(req_id, call_id, "post_storage.js", "read_posts", JSON.stringify(posts))
    .then(
       result => {
            Reply(req_id, call_id, ServiceName, result);
        }
    ); 
}
