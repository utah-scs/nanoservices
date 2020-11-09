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

function test(req_id, args) {
    print(DBSet("test.js", 1, str2ab("ok"), 0));
    let ab = DBGet("test.js", 1);
    let v = new DataView(ab);
    let version = v.getUint32(0, true);
    print(version);
    print("Get:");
    print(ab2str(ab));
    print(DBSet("test.js", 1, str2ab("ok"), version));

    let post = "W4Trb9x7cr7IOSc03KqSBCZnBsYRdtMSLh6d1fcOd5KSjria6Y3L4Idjb5WGGYvqTYjqbgsdOfxdV68qcIr8Cld7f4ALx8fxpANvHMMkKINLum8nVVOzCgvPC01F088oDwxXmYcN1ZgDVh5Yq7cdHEJ8e05Vkoevy3fFja9q9YU9BNSoiURasi6rgh2qN3CcKrYVlQ12zF4OLLeH1kd7Ca027oTQQFdsejAiBSW0HTSe16OEd1mDU4RFqW0gkKNq @username_588 @username_784 @username_761 @username_50 @username_58 @username_17 http://gCZyQhpIIHUthlDvZwPBfFSn2WC7hnljloCBCzoqeJ0Po8WZrP6ushWyqd7bUAny http://WJ8vg0BavUPis6DBanTJ28N0a35b8dEnV9IpIecEzqzp3B4MqH99U3SyEvKp3BLW http://Twjjvu5Bt4MGijEmVetOwDggTAbmeXPsFvt8BieauN2LB6xMYd1o2XWzBBnj7cs4 http://64kTWmGuJ8GwT84trn4q9OVqVVlRzfUNYwURqjUT2xozPLRIy0jCYvMytWdh2GE6";
//    return async_call(req_id, call_id, "text_service.js", "upload_text", post)
    return async_call(req_id, call_id, "user.js", "login", args)
    .then(
       result => {
            let rep = new Object();
            rep._status = 200;
            rep._message = result;
            let ret = JSON.stringify(rep);
	    Reply(req_id, ServiceName, ret);}
    );
}
