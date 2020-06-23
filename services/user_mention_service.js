function async_call(service, func, args) {
    return new Promise(function(resolve, reject) {
        Call(service, func,
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

function upload_user_mentions(mentions) {
    let length = mentions.length;
    let results = [];
    for (let i = 0; i < length; i++) {
        let username = mentions[i].substring(1);
        let userid = ab2str(DBGet("user_mention_service.js", username));
        results.push([username, userid]);
    }
//    print(results);
    return results;
}
