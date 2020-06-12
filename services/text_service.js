function upload_text(val) {
    print(val);
    let user_mentions = val.match(/@[a-zA-Z0-9-_]+/g);
//    print(user_mentions);
    let urls = val.match(/(http:\/\/|https:\/\/)([a-zA-Z0-9_!~*'().&=+$%-]+)/g);
//    print(urls);
    Call("user_mention_service.js", "upload_user_mentions", user_mentions);
    Call("url_shorten_service.js", "upload_urls", urls);
    return "OK\n";
}
