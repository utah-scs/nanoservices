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

function upload_text(req_id, call_id, val) {
    let user_mentions = val.match(/@[a-zA-Z0-9-_]+/g);
    let urls = val.match(/(http:\/\/|https:\/\/)([a-zA-Z0-9_!~*'().&=+$%-]+)/g);
    let mentions = JSON.stringify(user_mentions);
    async_call(req_id, call_id, "user_mention_service.js", "upload_user_mentions", mentions);

    let arg = JSON.stringify(urls);
    async_call(req_id, call_id, "url_shorten_service.js", "upload_urls", arg)
    .then(
       result => {
	       let updated_text = val;
               let updated_urls = JSON.parse(result);
	       for (let i = 0; i < urls.length; i++) {
                   updated_text = updated_text.replace(updated_urls.urls[i],
			                               updated_urls.shorturls[i]);
	       }
               async_call(req_id, call_id, "compose_post_service.js", "upload_text", updated_text)
	       .then(
                   result => {
	               Reply(call_id, ServiceName, "ok");
		   }
	       );
       }
    );
}
