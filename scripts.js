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

function setup() {
    if (!NewDB("user_mention_service"))
	print("Create database for user_mention_service failed.");
    if (!NewDB("url_service"))
	print("Create database for url_service failed.");
}

function user_register(userid, username) {
    DBSet("user_mention_service", username, str2ab(userid));
}

function user_mention_service(mentions) {
    let length = mentions.length;
    let results = [];
    for (let i = 0; i < length; i++) {
	let username = mentions[i].substring(1);
        let userid = ab2str(DBGet("user_mention_service", username));
	results.push([username, userid]);
    }
    print(results);
    return;
}

function url_service(arg) {
    print(arg);
    return;
}

function test(val) {
    print(val);
    let user_mentions = val.match(/@[a-zA-Z0-9-_]+/g);
//    print(user_mentions);
    let urls = val.match(/(http:\/\/|https:\/\/)([a-zA-Z0-9_!~*'().&=+$%-]+)/g);
//    print(urls);
    Call("user_mention_service", user_mentions);
    Call("url_service", urls);
    return "OK\n";
}
